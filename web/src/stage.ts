import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { DealGame, PRIZES, formatMoney } from './game';

interface CaseVisual {
  lid: THREE.Group;
  halo: THREE.Mesh;
  label: THREE.Mesh;
  hitbox: THREE.Mesh;
}

const CAMERA_PRESETS = [
  { name: '全景', position: [-23.5, 10.5, 0], target: [3, 2.55, 0], fov: 69 },
  { name: '中央桌', position: [-7.6, 3.1, 3.6], target: [-0.8, 1.2, 0], fov: 52 },
  { name: '箱阵', position: [-9.6, 5.75, -0.65], target: [4.9, 2.3, 0], fov: 51 },
  { name: '金额板', position: [-5, 5, 0], target: [4.2, 3.5, 7.6], fov: 80 },
] as const;

function verticalFov(horizontalFov: number, aspect: number): number {
  return Math.atan(Math.tan(horizontalFov * Math.PI / 360) / aspect) * 360 / Math.PI;
}

function textTexture(text: string, color = '#f7f7f5', background?: string): THREE.CanvasTexture {
  const canvas = document.createElement('canvas');
  canvas.width = 512;
  canvas.height = 128;
  const ctx = canvas.getContext('2d')!;
  if (background) {
    ctx.fillStyle = background;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
  }
  ctx.fillStyle = color;
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.font = '700 66px Arial, sans-serif';
  ctx.fillText(text, 256, 65, 485);
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.anisotropy = 4;
  return texture;
}

function makeTextPlane(text: string, width: number, height: number, color?: string, background?: string) {
  const material = new THREE.MeshBasicMaterial({
    map: textTexture(text, color, background),
    transparent: !background,
    side: THREE.DoubleSide,
    depthWrite: !!background,
  });
  const plane = new THREE.Mesh(new THREE.PlaneGeometry(width, height), material);
  plane.rotation.y = -Math.PI / 2;
  plane.scale.x = -1; // Cancel the set mirror so labels read left-to-right.
  return plane;
}

export class StudioStage {
  readonly cameraNames = CAMERA_PRESETS.map(item => item.name);
  readonly ready: Promise<void>;
  private scene = new THREE.Scene();
  // Blender's glTF exporter maps its +Y onto -Z, whereas UE shows +Y on screen-right.
  // Mirror the authored kit once so the approved UE left/right composition survives.
  private set = new THREE.Group();
  private renderer: THREE.WebGLRenderer;
  private camera = new THREE.PerspectiveCamera(69, 16 / 9, 0.08, 200);
  private target = new THREE.Vector3(...CAMERA_PRESETS[0].target);
  private wantedPosition = new THREE.Vector3(...CAMERA_PRESETS[0].position);
  private wantedTarget = this.target.clone();
  private wantedFov: number = CAMERA_PRESETS[0].fov;
  private currentCamera = 0;
  private caseVisuals: CaseVisual[] = [];
  private amountVisuals: THREE.Mesh[] = [];
  private amountLabels: THREE.Mesh[] = [];
  private hitboxes: THREE.Mesh[] = [];
  private raycaster = new THREE.Raycaster();
  private pointer = new THREE.Vector2();
  private lastTime = 0;
  private resizeObserver: ResizeObserver;
  private running = true;

  constructor(private host: HTMLElement, private onCaseClick: (number: number) => void) {
    this.scene.background = new THREE.Color('#050a12');
    this.scene.fog = new THREE.Fog('#050a12', 28, 65);
    this.set.scale.z = -1;
    this.scene.add(this.set);
    this.renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false, powerPreference: 'high-performance' });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.6));
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 2.15;
    this.host.appendChild(this.renderer.domElement);
    this.renderer.domElement.setAttribute('aria-label', 'Deal or No Deal 三维摄影棚。可以用下方数字按钮选箱。');
    this.camera.fov = verticalFov(this.wantedFov, this.camera.aspect);
    this.camera.position.copy(this.wantedPosition);
    this.camera.lookAt(this.target);

    this.scene.add(new THREE.HemisphereLight(0xcceaff, 0x172036, 2.7));
    const key = new THREE.DirectionalLight(0xffffff, 3.2);
    key.position.set(-7, 16, 7);
    this.scene.add(key);
    const blue = new THREE.PointLight(0x3bafff, 135, 30);
    blue.position.set(0, 7, 0);
    this.scene.add(blue);
    const warm = new THREE.PointLight(0xffaa55, 90, 26);
    warm.position.set(-5, 8, -9);
    this.scene.add(warm);

    this.renderer.domElement.addEventListener('click', this.handleClick);
    this.renderer.domElement.addEventListener('pointermove', this.handlePointerMove);
    this.resizeObserver = new ResizeObserver(() => this.resize());
    this.resizeObserver.observe(host);
    this.resize();
    this.ready = this.loadModel();
    requestAnimationFrame(this.animate);
  }

  get cameraIndex(): number { return this.currentCamera; }

  private async loadModel(): Promise<void> {
    const gltf = await new GLTFLoader().loadAsync(`${import.meta.env.BASE_URL}assets/deal-studio.glb`);
    const model = (name: string): THREE.Object3D => {
      const found = gltf.scene.getObjectByName(name);
      if (!found) throw new Error(`摄影棚部件缺失：${name}`);
      return found;
    };
    const chair = model('SM_StudioChair');
    const body = model('SM_BriefcaseBody');
    const lid = model('SM_BriefcaseLid');
    const prototypes = new Set(['SM_StudioChair', 'SM_BriefcaseBody', 'SM_BriefcaseLid']);
    for (const child of [...gltf.scene.children]) {
      if (!prototypes.has(child.name)) this.set.add(child);
    }
    this.addChairs(chair);
    this.addCases(body, lid);
    this.addAmountBoard();
  }

  private addChairs(prototype: THREE.Object3D): void {
    const placements: Array<[number, number, number, number]> = [];
    for (let row = 0; row < 5; row++) {
      for (let seat = 0; seat < 21; seat++) {
        placements.push([(-720 - row * 90) / 100, row * 0.28, -(-800 + seat * 80) / 100, 0]);
      }
    }
    for (const sign of [-1, 1]) {
      for (let row = 0; row < 4; row++) {
        for (let seat = 0; seat < 9; seat++) {
          placements.push([(-560 + seat * 90) / 100, row * 0.28,
            -sign * (810 + row * 88) / 100, sign * Math.PI / 2]);
        }
      }
    }
    prototype.updateWorldMatrix(true, true);
    prototype.traverse(part => {
      if (!(part instanceof THREE.Mesh)) return;
      const seats = new THREE.InstancedMesh(part.geometry, part.material, placements.length);
      const matrix = new THREE.Matrix4();
      const quaternion = new THREE.Quaternion();
      placements.forEach(([x, y, z, yaw], index) => {
        quaternion.setFromAxisAngle(new THREE.Vector3(0, 1, 0), yaw);
        matrix.compose(new THREE.Vector3(x, y, z), quaternion, new THREE.Vector3(1, 1, 1));
        matrix.multiply(part.matrixWorld);
        seats.setMatrixAt(index, matrix);
      });
      seats.instanceMatrix.needsUpdate = true;
      seats.frustumCulled = false;
      this.set.add(seats);
    });
  }

  private addCases(bodyPrototype: THREE.Object3D, lidPrototype: THREE.Object3D): void {
    const tiers = [
      { count: 6, x: 285, z: 55 },
      { count: 7, x: 405, z: 135 },
      { count: 7, x: 525, z: 215 },
      { count: 6, x: 645, z: 295 },
    ];
    let number = 1;
    for (const tier of tiers) {
      for (let index = 0; index < tier.count; index++) {
        const caseY = (index - (tier.count - 1) / 2) * 145;
        const group = new THREE.Group();
        group.position.set(tier.x / 100, (tier.z + 108) / 100, -caseY / 100);
        const body = bodyPrototype.clone();
        group.add(body);
        const lid = new THREE.Group();
        lid.position.set(-0.08, -0.17, 0);
        lid.add(lidPrototype.clone());
        group.add(lid);

        const label = makeTextPlane(String(number).padStart(2, '0'), 0.32, 0.18, '#111824', '#f0f7fc');
        label.position.set(-0.125, 0.02, 0);
        group.add(label);

        const halo = new THREE.Mesh(new THREE.BoxGeometry(0.22, 0.06, 0.60),
          new THREE.MeshBasicMaterial({ color: '#ffd075', transparent: true, opacity: 0.9 }));
        halo.position.set(-0.17, -0.38, 0);
        group.add(halo);

        const hitbox = new THREE.Mesh(new THREE.BoxGeometry(0.75, 0.72, 0.72),
          new THREE.MeshBasicMaterial({ transparent: true, opacity: 0, depthWrite: false }));
        hitbox.position.set(0, -0.02, 0);
        hitbox.userData.caseNumber = number;
        group.add(hitbox);
        this.hitboxes.push(hitbox);
        this.caseVisuals.push({ lid, halo, label, hitbox });
        this.set.add(group);
        number++;
      }
    }
  }

  private addAmountBoard(): void {
    const heading = makeTextPlane('AMOUNTS', 2.8, 0.40, '#fff2d3');
    heading.position.set(3.89, 6.67, -8.95);
    this.set.add(heading);
    for (let index = 0; index < 26; index++) {
      const leftColumn = index < 13;
      const row = index % 13;
      const z = -(8.95 + (leftColumn ? -0.98 : 0.98));
      const y = (603 - row * 46) / 100;
      const color = leftColumn ? '#147db7' : '#95571e';
      const tile = new THREE.Mesh(new THREE.PlaneGeometry(1.81, 0.35),
        new THREE.MeshBasicMaterial({ color, side: THREE.DoubleSide, transparent: true }));
      tile.rotation.y = -Math.PI / 2;
      tile.position.set(3.87, y, z);
      this.set.add(tile);
      const label = makeTextPlane(formatMoney(PRIZES[index]), 1.62, 0.27);
      label.position.set(3.852, y, z);
      this.set.add(label);
      this.amountVisuals.push(tile);
      this.amountLabels.push(label);
    }
  }

  update(game: DealGame): void {
    if (!this.caseVisuals.length) return;
    for (const visual of this.caseVisuals) {
      const number = this.caseVisuals.indexOf(visual) + 1;
      const state = game.cases[number - 1];
      const visuallyOpened = state.opened || (game.phase === 'gameover' &&
        (number === game.playerCase || (game.acceptedOffer === 0 && !state.opened)));
      visual.halo.visible = !visuallyOpened && (number === game.selected || number === game.playerCase);
      (visual.halo.material as THREE.MeshBasicMaterial).color.set(number === game.playerCase ? '#3bd6ff' : '#ffd075');
      (visual.label.material as THREE.MeshBasicMaterial).opacity = visuallyOpened ? 0.4 : 1;
      visual.hitbox.visible = game.canSelect(number);
    }
    const openAmounts = new Set(game.cases.filter(item => item.opened).map(item => item.amount));
    this.amountVisuals.forEach((tile, index) => {
      const opened = openAmounts.has(PRIZES[index]);
      (tile.material as THREE.MeshBasicMaterial).opacity = opened ? 0.12 : 1;
      (this.amountLabels[index].material as THREE.MeshBasicMaterial).opacity = opened ? 0.2 : 1;
    });
  }

  setCamera(index: number, immediate = false): void {
    if (index < 0 || index >= CAMERA_PRESETS.length) return;
    const preset = CAMERA_PRESETS[index];
    this.currentCamera = index;
    this.wantedPosition.set(preset.position[0], preset.position[1], preset.position[2]);
    this.wantedTarget.set(preset.target[0], preset.target[1], preset.target[2]);
    this.wantedFov = preset.fov;
    if (immediate) {
      this.camera.position.copy(this.wantedPosition);
      this.target.copy(this.wantedTarget);
      this.camera.fov = verticalFov(this.wantedFov, this.camera.aspect);
      this.camera.updateProjectionMatrix();
      this.camera.lookAt(this.target);
    }
  }

  private resize(): void {
    const width = Math.max(1, this.host.clientWidth);
    const height = Math.max(1, this.host.clientHeight);
    this.renderer.setSize(width, height, false);
    this.camera.aspect = width / height;
    this.camera.updateProjectionMatrix();
  }

  private pick(event: PointerEvent | MouseEvent): THREE.Intersection | undefined {
    const rect = this.renderer.domElement.getBoundingClientRect();
    this.pointer.set((event.clientX - rect.left) / rect.width * 2 - 1,
      -((event.clientY - rect.top) / rect.height * 2 - 1));
    this.raycaster.setFromCamera(this.pointer, this.camera);
    return this.raycaster.intersectObjects(this.hitboxes, false)[0];
  }

  private handleClick = (event: MouseEvent): void => {
    const hit = this.pick(event);
    if (hit) this.onCaseClick(hit.object.userData.caseNumber as number);
  };

  private handlePointerMove = (event: PointerEvent): void => {
    this.renderer.domElement.style.cursor = this.pick(event) ? 'pointer' : 'default';
  };

  private animate = (time: number): void => {
    if (!this.running) return;
    const dt = this.lastTime ? Math.min((time - this.lastTime) / 1000, 0.1) : 0.016;
    this.lastTime = time;
    const blend = 1 - Math.exp(-dt * 4.5);
    this.camera.position.lerp(this.wantedPosition, blend);
    this.target.lerp(this.wantedTarget, blend);
    this.camera.fov += (verticalFov(this.wantedFov, this.camera.aspect) - this.camera.fov) * blend;
    this.camera.updateProjectionMatrix();
    this.camera.lookAt(this.target);
    this.caseVisuals.forEach((visual, index) => {
      const opened = !visual.hitbox.visible && !visual.halo.visible &&
        (visual.label.material as THREE.MeshBasicMaterial).opacity < 1;
      visual.lid.rotation.z += ((opened ? -1.83 : 0) - visual.lid.rotation.z) * blend;
    });
    this.renderer.render(this.scene, this.camera);
    requestAnimationFrame(this.animate);
  };

  dispose(): void {
    this.running = false;
    this.resizeObserver.disconnect();
    this.renderer.dispose();
  }
}
