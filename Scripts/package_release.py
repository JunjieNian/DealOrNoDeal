"""Archive the portable runtime without local test output or debug symbols."""
import hashlib
import json
from pathlib import Path
import zipfile
import argparse

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--version', default='0.4.2')
args = parser.parse_args()
version = args.version
package = root / f'Builds/DealOrNoDealStage-Studio-{version}'
runtime = package / 'Windows'
archive = root / f'Builds/DealOrNoDealStage-Studio-{version}-Windows.zip'
files = sorted(p for p in runtime.rglob('*') if p.is_file()
               and 'Saved' not in p.relative_to(runtime).parts
               and p.suffix.lower() not in {'.pdb', '.log'}
               and p.name != 'Manifest_DebugFiles_Win64.txt')
assert (runtime / 'DealOrNoDealStage.exe') in files
assert any(p.suffix == '.ucas' for p in files)
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as output:
    for path in files:
        output.write(path, path.relative_to(package).as_posix())
    output.writestr('START-HERE.txt',
        f'Deal or No Deal - Studio Experience {version}\n\n'
        '解压整个压缩包，打开 Windows 文件夹，双击 DealOrNoDealStage.exe。\n'
        '请保留完整 Windows 文件夹，不能只复制启动程序。无需安装 Unreal Editor。\n\n'
        '金额板与两侧阶梯已修正交叠，扶手固定在踏步上。\n'
        'Cam 4 使用左侧选箱面板，金额板底部保持可见。\n'
        '1/2/3/4 切换镜头；点击编号预选，再确认；Enter 确认；Esc 暂停。\n'
        'D/N 接受或拒绝报价；A 自动镜头；T 展示速度；M 声音；F11 全屏。\n\n'
        'Extract the full archive and run Windows/DealOrNoDealStage.exe.\n'
        'Keep the entire Windows folder together. Unreal Editor is not required.\n'
        'Source and verification: https://github.com/JunjieNian/DealOrNoDeal\n')
with zipfile.ZipFile(archive) as check:
    assert check.testzip() is None, 'ZIP integrity check failed'
digest = hashlib.file_digest(archive.open('rb'), 'sha256').hexdigest()
checksum = archive.with_suffix('.sha256')
checksum.write_text(f'{digest}  {archive.name}\n', encoding='ascii')
report = dict(archive=archive.name, archive_bytes=archive.stat().st_size,
              sha256=digest, runtime_files=len(files), runtime_bytes=sum(p.stat().st_size for p in files),
              zip_crc_check='PASS')
(root / f'Saved/Archive-{version}.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report), flush=True)
