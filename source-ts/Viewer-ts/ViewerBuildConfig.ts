/**
 * Viewer building configuration.
 */
export const ViewerBuildConfig = {
  /**
   * List of WASM configurations.
   */
  // wasmConfigs: 'wasm64-pthread wasm64 wasm32-pthread wasm32'.split(' ')
  wasmConfigs: '${OCCTV_WASM_CONFIGS}'.split(' '),
};
