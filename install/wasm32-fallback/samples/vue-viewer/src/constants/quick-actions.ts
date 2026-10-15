export interface QuickAction {
  icon: string;
  label: string;
  action: string;
}

export const QUICK_ACTIONS: QuickAction[] = [
  { icon: 'mdi-fit-to-page-outline', label: '自适应', action: 'fitAll' },
  { icon: 'mdi-cube-outline', label: '视图立方体', action: 'viewCube' },
  { icon: 'mdi-rotate-3d-variant', label: '等轴测', action: 'axo' },
  { icon: 'mdi-broom', label: '清理', action: 'clearScene' }
];
