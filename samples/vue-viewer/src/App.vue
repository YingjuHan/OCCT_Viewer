<script setup lang="ts">
import Viewer from './components/Viewer.vue';
import ControlPanel from './components/ControlPanel.vue';
import ToolDock from './components/ToolDock.vue';
import CaePostPanel from './components/cae/CaePostPanel.vue';
import CaeProbePanel from './components/cae/CaeProbePanel.vue';
import LwvAppBar from './components/layout/LwvAppBar.vue';
import QuickRail from './components/layout/QuickRail.vue';
import ViewerMessageToast from './components/ViewerMessageToast.vue';
import { QUICK_ACTIONS } from './constants/quick-actions';
import { useDockPanels } from './composables/useDockPanels';
import { useViewerLogs } from './composables/useViewerLogs';
import { ViewerService } from "./services/viewer.service";
import './assets/control-panel.css';

const viewerService = new ViewerService();
const { dockState, minimizedPanels, togglePanel, minimizePanel, restorePanel } = useDockPanels();
const { loadLog, handleFilesLoaded } = useViewerLogs();

const handleRailAction = (action: string) => {
  switch (action) {
    case 'fitAll':
      viewerService.fitAll(true);
      break;
    case 'viewCube':
      viewerService.toggleViewCube();
      break;
    case 'axo':
      viewerService.setAxoView(true);
      break;
    case 'clearScene':
      viewerService.clearScene(true);
      break;
    default:
      console.info(`[rail] '${action}' 灏氭湭瀹炵幇`);
  }
};
</script>

<template>
  <v-app>
    <LwvAppBar
      :viewerService="viewerService"
      @filesLoaded="handleFilesLoaded"
      @toggleRight="togglePanel('right')"
    />

    <v-main class="lwv-main">
      <div class="lwv-layout">
        <QuickRail :actions="QUICK_ACTIONS" @trigger="handleRailAction">
          <v-divider />
          <v-btn icon variant="text" size="small" @click="togglePanel('left')">
            <v-icon>{{ dockState.left.open ? 'mdi-panel-left-close' : 'mdi-panel-left' }}</v-icon>
          </v-btn>
        </QuickRail>

        <v-navigation-drawer
          class="dock-panel"
          v-model="dockState.left.open"
          location="left"
          :width="300"
          mode="floating"
          :scrim="false"
          :touchless="true"
          border="none"
        >
          <div class="dock-header">
            <span>{{ dockState.left.title }}</span>
            <v-btn icon size="small" variant="text" @click="minimizePanel('left')">
              <v-icon>mdi-window-minimize</v-icon>
            </v-btn>
          </div>
          <ToolDock :viewerService="viewerService" />
          <v-divider class="my-4" />
          <CaePostPanel :viewerService="viewerService" />
          <v-divider class="my-4" />
          <CaeProbePanel :viewerService="viewerService" />
        </v-navigation-drawer>

        <div class="lwv-stage">
          <Viewer class="viewer" :viewerService="viewerService" @filesLoaded="handleFilesLoaded" />
          <v-alert
            v-if="loadLog.length"
            class="lwv-log"
            border="start"
            type="info"
            variant="tonal"
          >
            <div class="load-log-title">最近导入</div>
            <div class="load-log-items">
              <div
                v-for="(item, index) in loadLog"
                :key="index"
                class="load-log-item"
              >
                <v-chip :color="item.success ? 'success' : 'error'" size="small" variant="flat">
                  {{ item.fileName }}
                </v-chip>
                <span class="load-log-status">{{ item.message }}</span>
              </div>
            </div>
          </v-alert>
        </div>

        <v-navigation-drawer
          class="dock-panel"
          v-model="dockState.right.open"
          location="right"
          :width="360"
          mode="floating"
          :scrim="false"
          :touchless="true"
          border="none"
        >
          <div class="dock-header">
            <span>{{ dockState.right.title }}</span>
            <v-btn icon size="small" variant="text" @click="minimizePanel('right')">
              <v-icon>mdi-window-minimize</v-icon>
            </v-btn>
          </div>
          <v-divider class="mb-2" />
          <ControlPanel :viewerService="viewerService" />
        </v-navigation-drawer>
      </div>
    </v-main>

    <v-bottom-navigation
      v-if="minimizedPanels.length"
      grow
      color="primary"
      class="lwv-bottom"
    >
      <v-btn
        v-for="panel in minimizedPanels"
        :key="panel.key"
        variant="text"
        @click="restorePanel(panel.key)"
      >
        {{ panel.title }}
      </v-btn>
    </v-bottom-navigation>
      <ViewerMessageToast :viewerService="viewerService" />
  </v-app>
</template>

<style scoped>
.lwv-main {
  background: linear-gradient(180deg, #161c24 0%, #0f1115 60%, #0a0c10 100%);
  min-height: 100vh;
}

.lwv-layout {
  display: flex;
  gap: 12px;
  padding: 18px;
  position: relative;
  height: calc(100vh - 96px);
}

.lwv-stage {
  flex: 1;
  border-radius: 24px;
  overflow: hidden;
  position: relative;
  background: #050505;
  box-shadow: 0 30px 50px rgba(0, 0, 0, 0.55);
}

.viewer {
  width: 100%;
  height: 100%;
}

.dock-panel {
  background: rgba(23, 26, 33, 0.95);
  backdrop-filter: blur(18px);
  border-radius: 20px;
  margin: 0 8px;
  box-shadow: 0 24px 48px rgba(0, 0, 0, 0.45);
}

.dock-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-weight: 600;
  padding-bottom: 4px;
}

.lwv-log {
  position: absolute;
  bottom: 24px;
  left: 24px;
  max-width: 420px;
  background: rgba(13, 15, 20, 0.85) !important;
  backdrop-filter: blur(12px);
}

.load-log-title {
  font-weight: 600;
  margin-bottom: 4px;
}

.load-log-items {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.load-log-item {
  display: flex;
  align-items: center;
  gap: 6px;
}

.load-log-status {
  font-size: 0.82rem;
  color: rgba(255, 255, 255, 0.7);
}

.lwv-bottom {
  border-top: 1px solid rgba(255, 255, 255, 0.08);
}

@media (max-width: 960px) {
  .lwv-layout {
    flex-direction: column;
    height: auto;
  }

  .lwv-rail {
    flex-direction: row;
    width: auto;
    border-radius: 16px;
  }

  .dock-panel {
    width: 100% !important;
    margin: 12px 0;
  }

  .lwv-stage {
    height: 60vh;
  }
}
</style>





