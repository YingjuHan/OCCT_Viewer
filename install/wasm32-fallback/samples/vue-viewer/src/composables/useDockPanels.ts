import { computed, reactive } from 'vue';

export type PanelKey = 'left' | 'right';

interface PanelState {
  title: string;
  open: boolean;
  minimized: boolean;
}

const dockState = reactive<Record<PanelKey, PanelState>>({
  left: { title: '工具', open: true, minimized: false },
  right: { title: '控制面板', open: true, minimized: false }
});

const minimizedPanels = computed(() => Object.entries(dockState)
  .filter(([, cfg]) => cfg.minimized)
  .map(([key, cfg]) => ({ key: key as PanelKey, title: cfg.title })));

export function useDockPanels() {
  const togglePanel = (key: PanelKey): void => {
    const panel = dockState[key];
    panel.open = !panel.open;
    panel.minimized = !panel.open;
  };

  const minimizePanel = (key: PanelKey): void => {
    dockState[key].open = false;
    dockState[key].minimized = true;
  };

  const restorePanel = (key: PanelKey): void => {
    dockState[key].open = true;
    dockState[key].minimized = false;
  };

  return {
    dockState,
    minimizedPanels,
    togglePanel,
    minimizePanel,
    restorePanel
  };
}
