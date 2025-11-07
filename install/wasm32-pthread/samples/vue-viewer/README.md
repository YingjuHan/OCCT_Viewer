Steps to start:
1. Install Node.js (if copied from https://his.huawei.com/gsam/index.html#/ToolInfo?toolId=1420375076053712896, add path to node.exe to PATH)
2. Configure the npm repository as described in https://confluence-msc.rnd.huawei.com/display/CDT/Tiny+Vue.+Install+guide (step 5)
3. Open VSCode in vue-viewer folder
4. Open VSCode's terminal
5. Call "npm install"
6. Call "npm run serve"
7. Open http://localhost:8080/

To use a local build of c++ viewer replace "@hw/SCwebviewer": "git+ssh://git@rnd-gitlab-msc.huawei.com:2222/industrial-software-kernel-and-platform-project-ru/simcloud-viewer-npm.git#SHA" with "@hw/SCwebviewer": "file:../../work/scviewer-wasm32" in package.json and call npm install.
