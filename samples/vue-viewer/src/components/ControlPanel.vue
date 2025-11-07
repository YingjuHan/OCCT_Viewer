<script setup lang="ts">
import { nextTick, onMounted } from 'vue';
import { ViewerService } from '../services/viewer.service';
import { ControlPanel } from '../control-panel/SCcontrols.js';
import { ImportPanel } from '../control-panel/Panels/ImportPanel.js';
import { ScenePanel } from '../control-panel/Panels/ScenePanel.js';
import { ShapePanel } from '../control-panel/Panels/ShapePanel.js';
import { BuildersPanel } from '../control-panel/Panels/BuildersPanel.js';
import { MeshPanel } from '../control-panel/Panels/MeshPanel.js';
import { ClippingPanel } from '../control-panel/Panels/ClippingPanel.js';
import { SettingsPanel } from '../control-panel/Panels/SettingsPanel.js';
// import { TemplatePanel } from '../control-panel/Panels/TemplatePanel.js';

const props = defineProps<{
  viewerService: ViewerService
}>();

const panelId = 'cp';

onMounted(async () => {
  await props.viewerService.whenReady();
  await nextTick();

  const panel = new ControlPanel(panelId);
  panel.addPanel(new ImportPanel(props.viewerService));
  panel.addPanel(new ScenePanel(props.viewerService));
  panel.addPanel(new ShapePanel(props.viewerService));
  panel.addPanel(new BuildersPanel(props.viewerService));
  panel.addPanel(new MeshPanel(props.viewerService));
  panel.addPanel(new ClippingPanel(props.viewerService));
  panel.addPanel(new SettingsPanel(props.viewerService));
  // panel.addPanel(new TemplatePanel(props.viewerService));

  panel.redraw();
  panel.switchPanel('Import');
});
</script>

<template>
  <div class="control-panel-shell">
    <div :id="`${panelId}Header`" class="cp-header"></div>
    <div :id="`${panelId}Body`" class="cp-body"></div>
  </div>
</template>
