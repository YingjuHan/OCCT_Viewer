/**
 * Viewer building configuration.
 */
export const ViewerBuildConfig = {
  /**
   * List of WASM configurations.
   */
  // wasmConfigs: 'wasm64-pthread wasm64 wasm32-pthread wasm32'.split(' ')
  wasmConfigs: '${OCCTV_WASM_CONFIGS}'.split(' '),
  /**
   * STEP reader availability.
   */
  useStep: (function(): boolean {
    const aFlag: string = '${OCCTV_USE_STEP}';
    return aFlag === 'ON';
  })(),
  /**
   * IGES reader availability.
   */
  useIges: (function(): boolean {
    const aFlag: string = '${OCCTV_USE_IGES}';
    return aFlag === 'ON';
  })(),
};
