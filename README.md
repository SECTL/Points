<div align="center">
  <h1>FastPoints</h1>
  <img src="resource/icon/Frame%201%20(1).svg" alt="icon" width="400">
</div>

一款全新的积分管理系统，为班级教学场景设计，也可在家庭等场景使用。 配套完整的班级管理制度，保障积分制度的实施。

**本项目还在开发中，目前功能不完善，请勿使用。**

## **注意**
***尽管代码以GPL v3开源，但图标遵循LICENSE_ICON。***

## 亮点
- 精心设计的UI，方便、优雅
- 配套有开发者亲自精心设计的班级管理制度，保障教育场景中积分制度的实施
- 采用`c++`语言和`slint`UI框架，快、稳、精炼

## 使用方式
### 普通用户
请点击本网页上的Release字样，选择你需要的版本（建议使用最新正式版），找到与你的操作系统相匹配的安装包或压缩包，点击文件名下载。

### 开发者

```bash
# 克隆仓库
git clone https://github.com/SECTL/Points.git
cd Points

# 配置项目（自动下载 slint 依赖）
xmake config

# 编译
xmake build

# 运行
xmake run
```

其他常用命令
- 清理构建产物：`xmake clean`
- 以 Release 模式构建：`xmake config --mode=release && xmake build`

## 鸣谢
- SECTL思拓创联，智慧教育行业先锋，为本项目提供了不菲支持
- ShiMingXuanSimon，项目的发起者
- chenjintang-shrimp，为项目付出巨大心血、提供巨大技术帮助
