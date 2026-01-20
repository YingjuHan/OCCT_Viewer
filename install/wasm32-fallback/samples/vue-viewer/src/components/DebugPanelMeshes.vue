<script lang="ts">

import { defineComponent, PropType } from "vue"
import { ViewerService } from "../services/viewer.service";
import { CaeMeshObjectPrsMode, ICaeMeshPropInfo, ObjectId } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';

export default defineComponent({
    props: {
        viewerService: {
            type: Object as PropType<ViewerService>,
            required: true
        }
    },
    data() {
        return {
            lastLoadedMesh: "",
            currentProperty: "",
            currentDisplayMode: CaeMeshObjectPrsMode.FlatShaded
        }
    },
    methods: {
        loadMeshTestModel() {
            this.viewerService.loadMeshUrlAsync("models/gmsh/buckling22.pos").then((result) => {
                if (result) {
                    this.lastLoadedMesh = result;

                    this.currentProperty = "";
                    let aProperties = this.getProperties();
                    if (aProperties.length > 0) {
                        this.currentProperty = aProperties[0].propId;
                        this.updateActiveProperty();
                    }

                    this.currentDisplayMode = CaeMeshObjectPrsMode.FlatShaded;
                    this.updateDisplayMode();
                    this.viewerService.fitAll(true);
                    console.log("Model has been loaded");
                }
            });
        },

        updateActiveProperty() {
            this.viewerService.setActiveMeshProperty (this.lastLoadedMesh, this.currentProperty, true);
        },

        getProperties() : Array<ICaeMeshPropInfo>{
            return this.viewerService.getMeshProperties (this.lastLoadedMesh);
        },

        updateDisplayMode() {
            this.viewerService.setMeshesDisplayMode(this.currentDisplayMode, true);
        }
    }
})

</script>

<template>
    <div class="debug">
        <p>Meshes</p>
        <button @click="loadMeshTestModel">Load Mesh Test Model</button>
        <select v-model="currentProperty" @change="updateActiveProperty()">
            <option value="">None</option>
            <option v-for="property in getProperties()" :value="property.propId" :key="property.propId">{{ property.name }}</option>
        </select>
        <select v-model="currentDisplayMode" @change="updateDisplayMode()">
            <option :value="0">MeshEdges</option>
            <option :value="1">FlatShaded</option>
            <option :value="2">MeshEdgesFlatShaded</option>
            <option :value="3">MeshShrinked</option>
        </select>
    </div>
</template>

<style scoped>
.debug {
    display: flex;
    flex-direction: column;
}
</style>