## Vue Viewer Sample

基于 `@hyj/SCwebviewer` �?Vue 示例，现已整�?`samples/html` 中的全部控制面板与调试能力，并以 Vuetify 打�?Onshape 风格的工作台，可在浏览器中完整体�?CAD/CAE 数据的导入、浏览与诊断流程�?
### 快速运�?
```bash
cd samples/vue-viewer
npm install
npm run serve
```

### 功能亮点

- Vuetify Onshape UI：顶部命令条 + 左右浮动吸附面板，均支持最小化到底部标签栏，移动端自动折叠�?- 顶部导入体验：`v-file-input` 支持多文�?+ SCwebviewer 自动格式探测，配合快速场景按钮（Fit/Clear/ViewCube）�?- 左侧工具 Dock：用 Vuetify 按钮组封装相机视角、场景运维以及一键加载示例模型，对标 Onshape 快速指令�?- 右侧控制面板：承�?Import / Scene / Shape / Builders / Mesh / Clipping / Settings 全套 HTML Panel，行为与 `samples/html` 完全一致�?\n- CAE �������������ר����壬����ʾ���������л����ԡ���ʾģʽ���ڵ�/��Ԫ�����α�����������������ʾ��\n### 使用建议

1. 受益�?`InteractiveCaeMesh.probeFileFormat` �?`InteractiveShape.probeFileFormat`，无需关心扩展名即可导�?SCwebviewer 支持的全部格式�?2. MDBPROP 属性文件需要先加载对应网格模型后再导入，系统会将属性挂载到最近加载的 Mesh 上�?3. 面板依赖 `sceneChanged` 事件刷新列表，若在代码中直接操作 viewer，请记得 `window.dispatchEvent(new Event('sceneChanged'))`�?4. 大型模型加载建议开启浏览器 DevTools，若有报错可在控制台查看触发点；如需减小 bundle，可按需裁剪 Vuetify 组件或改用图�?CDN�?
