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
    useStep: (function () {
        const aFlag = 'OFF';
        return aFlag === 'ON';
    })(),
    /**
     * IGES reader availability.
     */
    useIges: (function () {
        const aFlag = 'OFF';
        return aFlag === 'ON';
    })(),
};
//# sourceMappingURL=ViewerBuildConfig.js.map