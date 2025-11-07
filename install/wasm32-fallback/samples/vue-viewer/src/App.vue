<script setup lang="ts">
import { defineComponent, ref } from 'vue'
import Viewer from './components/Viewer.vue'
import DebugPanelScene from './components/DebugPanelScene.vue'
import DebugPanelShapes from './components/DebugPanelShapes.vue'
import DebugPanelMeshes from './components/DebugPanelMeshes.vue'
import { ViewerService } from "./services/viewer.service";

const viewerService = new ViewerService();

</script>

<template>
  <div class="viewer-div">
    <div class="debug-panel">
      <DebugPanelScene class="debug-panel-scene" :viewerService="viewerService"></DebugPanelScene>
      <DebugPanelShapes class="debug-panel-shapes" :viewerService="viewerService"></DebugPanelShapes>
      <DebugPanelMeshes class="debug-panel-meshes" :viewerService="viewerService"></DebugPanelMeshes>
    </div>
    <Viewer class="viewer" :viewerService="viewerService"></Viewer>
  </div>
</template>

<style scoped>
.viewer-div {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: row;
}
.debug-panel {
  width: 100px;
  vertical-align: top;
  padding: 10px;
  display: flex;
  flex-direction: column;
}
.debug-panel-scene {
  flex: 0 0;
}
.debug-panel-shapes {
  flex: 0 0;
}
.debug-panel-meshes {
  flex: 0 0;
}
.viewer {
  flex: 1;
}
</style>