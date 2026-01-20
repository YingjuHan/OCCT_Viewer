/**
 * Viewer building configuration.
 */
export const ViewerBuildConfig = {
  /**
   * List of WASM configurations.
   */
  // wasmConfigs: 'wasm64-pthread wasm64 wasm32-pthread wasm32'.split(' ')
  wasmConfigs: 'wasm32'.split(' '),
  /**
   * STEP reader availability.
   */
  useStep: (function(): boolean {
    const aFlag: string = 'OFF';
    return aFlag === 'ON';
  })(),
  /**
   * IGES reader availability.
   */
  useIges: (function(): boolean {
    const aFlag: string = 'OFF';
    return aFlag === 'ON';
  })(),
};
