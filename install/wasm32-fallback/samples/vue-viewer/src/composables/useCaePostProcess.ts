import { onBeforeUnmount, onMounted, ref, watch } from 'vue';
import type { ObjectId, ICaeMeshPropInfo } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import { CaeMeshObjectPrsMode } from '@hyj/SCwebviewer/wasm32/SCwebviewer.js';
import type { ViewerService } from '../services/viewer.service';

export interface MeshOption {
    id: ObjectId;
    label: string;
}

export interface MeshElementStatus {
    elements3d: boolean;
    elements2d: boolean;
    nodes: boolean;
}

export function useCaePostProcess(viewerService: ViewerService) {
    const meshes = ref<MeshOption[]>([]);
    const selectedMeshId = ref<ObjectId | ''>('');
    const meshProperties = ref<ICaeMeshPropInfo[]>([]);
    const selectedProperty = ref('');
    const displayMode = ref<CaeMeshObjectPrsMode>(CaeMeshObjectPrsMode.FlatShaded);
    const elementStatus = ref<MeshElementStatus>({ elements3d: true, elements2d: true, nodes: true });
    const deformationScale = ref(1);

    const refreshMeshes = () => {
        meshes.value = viewerService.listMeshSummaries();
        if (meshes.value.length === 0) {
            selectedMeshId.value = '';
            meshProperties.value = [];
            return;
        }
        if (!selectedMeshId.value) {
            selectedMeshId.value = meshes.value[0].id;
        } else if (!meshes.value.find(mesh => mesh.id === selectedMeshId.value)) {
            selectedMeshId.value = meshes.value[0].id;
        }
        refreshMeshDetails();
    };

    const refreshMeshDetails = () => {
        if (!selectedMeshId.value) {
            meshProperties.value = [];
            return;
        }
        meshProperties.value = viewerService.getMeshProperties(selectedMeshId.value);
        if (meshProperties.value.length > 0 && !selectedProperty.value) {
            selectedProperty.value = meshProperties.value[0].propId;
            viewerService.setActiveMeshProperty(selectedMeshId.value, selectedProperty.value, true);
        }
        const status = viewerService.getMeshElementsDisplayingStatus(selectedMeshId.value);
        elementStatus.value = { ...status };
        deformationScale.value = viewerService.getMeshDefScaleCoeff(selectedMeshId.value) || 1;
    };

    const handleSceneChanged = () => refreshMeshes();

    onMounted(() => {
        refreshMeshes();
        window.addEventListener('sceneChanged', handleSceneChanged as EventListener);
    });

    onBeforeUnmount(() => {
        window.removeEventListener('sceneChanged', handleSceneChanged as EventListener);
    });

    watch(selectedMeshId, () => {
        if (selectedMeshId.value) {
            selectedProperty.value = '';
            refreshMeshDetails();
        }
    });

    watch(selectedProperty, (prop) => {
        if (prop && selectedMeshId.value) {
            viewerService.setActiveMeshProperty(selectedMeshId.value, prop, true);
        }
    });

    watch(displayMode, (mode) => {
        viewerService.setMeshesDisplayMode(mode, true);
    });

    watch(elementStatus, (status) => {
        if (!selectedMeshId.value) {
            return;
        }
        viewerService.setMeshElementsDisplayingStatus(selectedMeshId.value, status);
        viewerService.requestViewUpdate();
    }, { deep: true });

    watch(deformationScale, (value) => {
        if (!selectedMeshId.value) {
            return;
        }
        viewerService.setMeshDefScaleCoeff(selectedMeshId.value, value);
        viewerService.requestViewUpdate();
    });

    return {
        meshes,
        selectedMeshId,
        meshProperties,
        selectedProperty,
        displayMode,
        elementStatus,
        deformationScale
    };
}
