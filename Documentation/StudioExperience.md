# Studio Experience 0.4.1

这是在原有 2006–2008 年 Culver Studios 风格布局基础上的演播室场景迭代。
保留中央游戏台、6+7+7+6 的 26 箱阶梯、右侧金额屏、左侧银行家高台和 U 形空观众区。
没有人物、人体替身或观众角色。尺寸继续沿用项目的设计假设，不代表对真实摄影棚的测绘。

## 场景与源文件

- `ArtSource/DealStudio.blend`：可继续编辑的 Blender 场景，按功能模块命名。
- `ArtSource/build_studio.py`：确定性建模生成器；设计坐标使用厘米，输出模型使用米。
- `ArtSource/Export/`：12 个 FBX 模型及材质参数。
- `Content/Studio/`：Unreal 使用的模型、14 种材质和 4 个原创提示音。
- `Content/Maps/MainStage.umap`：10 个独立游戏模块、9 组实体布景、灯光及后期设置。

实体布景包含连续曲面拱门、分层夜景建筑、带金属包边和灯带的阶梯、展箱支架、座椅及看台踏步、扶手、桁架、灯头、带烟色玻璃与百叶的银行家包厢，以及带电话、按键、卷线和红色按钮的玻璃游戏台。
箱盖可以实际旋转打开；已打开的箱体保留在展台上。游戏台红色按钮是布景道具，成交通过屏幕按钮或键盘操作。

## 完整流程

1. 欢迎页进入游戏。
2. 点击箱子或编号面板预选，再次点击同一箱子或点击确认按钮锁定；方向键和 Enter 同样可用。Cam 4 使用左侧面板，完整避让金额屏；其他镜头使用底部选箱栏。
3. 按 6、5、4、3、2、1、1、1、1 的每轮数量开箱。自己的箱子不能提前打开。
4. 开箱金额显示后继续。默认停留约 2.8 秒；Space 可以在短暂展示后跳过，快速模式约 1.1 秒。
5. 来电时可以点击接听或按 Enter，也会在短暂来电后自动进入报价。
6. Deal 需要再次确认；可以返回报价。No Deal 继续下一轮，没有决策倒计时。
7. 最后两个箱子可以保留自己的箱子，也可以换箱。
8. 结算显示实际赢得的游戏金额、箱内金额和收到的报价次数，然后可以重玩。

报价按剩余金额平均值乘随轮次递增的系数计算，用于单机游戏节奏，不是电视节目的真实报价算法。全部金额仅为游戏内虚拟结果。

## 操作

| 操作 | 按键 |
| --- | --- |
| 切换候选箱 | 左 / 右 |
| 确认、接听、继续金额展示 | Enter / Space |
| 接受报价；最后两箱保留 | D |
| 拒绝报价；最后两箱换箱 | N |
| 结算后重玩 | R |
| 暂停菜单；取消成交确认 | Esc |
| 全景 / 游戏台 / 箱阵 / 金额屏 | 1 / 2 / 3 / 4 |
| 循环镜头 | C |
| 自动镜头开关 | A |
| 展示 / 快速节奏 | T |
| 声音开关 | M |
| High / Epic 画质 | F5 |
| 全屏切换 | F11 / Alt+Enter |

暂停菜单会挂起开箱与来电计时，阻止游戏决策；可以恢复、设置偏好、开始新游戏或退出。新游戏按钮明确提示会丢弃本局。游戏进行中按 R 不会意外重置。
声音、节奏和自动镜头偏好在同一次启动的重玩间保留；不提供跨启动存档。

## 再生成

1. 使用 Blender 4.5 运行 `ArtSource/build_studio.py`，生成源场景与 FBX。
2. 编译 `DealOrNoDealStageEditor Win64 Development`。
3. 用 UnrealEditor-Cmd 运行 `Content/Python/build_studio.py`，导入资源并再生成地图。导入声音时保持音频模块启用。
4. 仅调整场景或后期而不重新导入时，追加 `-StudioSkipImport`。

已有 Unreal 材质会复用，便于手工调整。脚本不删除被运行时类引用的材质表达式，避免 UE 5.7 的 rooted-expression 断言。
FBX 从 Blender 导入时 Y 坐标会反射；布景 Actor 统一使用 `(1,-1,1)` 缩放恢复项目约定的右侧金额屏。

## 验证入口

当前可运行包：`Builds/DealOrNoDealStage-Studio-0.4.1/Windows`。远端下载见 [v0.4.1 Release](https://github.com/JunjieNian/DealOrNoDeal/releases/tag/v0.4.1)。解压后保留整个 Windows 文件夹，启动其中的 `DealOrNoDealStage.exe`。

- `Scripts/Verify-Cam4Package.ps1`：验证实际打包程序的三个分辨率布局、选箱命中框、金额格避让、镜头切换及游戏流程。
- `-DealBoardLayoutTest -seconds=18`：验证 Cam 4 全部 26 个金额格无遮挡，以及预选、确认、开箱、禁用已选/已开箱和切回底部栏；保存四个阶段截图。
- `-DealAutoTest`：完整拒绝报价并保留原箱路线。
- `-DealAutoAcceptTest`：第一轮报价成交路线。
- `-DealExperienceTest -DealSeed=20260906`：固定种子的选择保护、成交确认、重玩、保留/换箱、来电及暂停计时回归。
- `-DealUIValidation -seconds=62`：通过当前分辨率下实际生成的按钮命中框驱动完整界面流程，并逐阶段保存截图。
- `-DealCapturePreview -CaptureName=Name -CaptureDelay=5`：保存当前游戏截图。
- `-DealCleanPreview`：隐藏 HUD，便于场景画面检查。
- `-StageCamera=0/1/2/3`：指定启动镜头。

截图来自引擎实际渲染；建模未使用生成式图片或外部商用素材。
