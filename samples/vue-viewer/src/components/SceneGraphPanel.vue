<script setup lang="ts">
import { computed, onMounted, ref } from 'vue';
import { ViewerService } from '../services/viewer.service';

type SceneGraphNode = {
  id: string;
  name: string;
  type: string;
  parentId: string | null;
  children: string[];
};

type SceneGraph = {
  specVersion: string;
  rootNodeId: string;
  nodes: SceneGraphNode[];
};

const props = defineProps<{
  viewerService: ViewerService;
}>();

const search = ref('');
const graph = ref<SceneGraph | null>(null);
const isLoading = ref(false);
const expanded = ref<Set<string>>(new Set(['root']));

const nodesById = computed(() => {
  const map = new Map<string, SceneGraphNode>();
  if (!graph.value) return map;
  for (const node of graph.value.nodes) {
    map.set(node.id, node);
  }
  return map;
});

type FlatRow = {
  id: string;
  depth: number;
  node: SceneGraphNode;
  isExpanded: boolean;
  hasChildren: boolean;
};

const filteredFlatRows = computed<FlatRow[]>(() => {
  if (!graph.value) return [];
  const q = search.value.trim().toLowerCase();
  const byId = nodesById.value;
  const root = byId.get(graph.value.rootNodeId);
  if (!root) return [];

  const result: FlatRow[] = [];

  const matches = (node: SceneGraphNode) => {
    if (!q) return true;
    return (node.name ?? '').toLowerCase().includes(q) || node.id.toLowerCase().includes(q) || (node.type ?? '').toLowerCase().includes(q);
  };

  const subtreeHasMatch = (nodeId: string): boolean => {
    const node = byId.get(nodeId);
    if (!node) return false;
    if (matches(node)) return true;
    return (node.children ?? []).some(subtreeHasMatch);
  };

  const walk = (nodeId: string, depth: number) => {
    const node = byId.get(nodeId);
    if (!node) return;

    if (!subtreeHasMatch(nodeId)) return;

    const hasChildren = (node.children?.length ?? 0) > 0;
    const isExpanded = expanded.value.has(nodeId);
    result.push({ id: nodeId, depth, node, isExpanded, hasChildren });

    if (!hasChildren || !isExpanded) return;
    for (const childId of node.children) {
      walk(childId, depth + 1);
    }
  };

  walk(root.id, 0);
  return result;
});

const refresh = async () => {
  isLoading.value = true;
  try {
    await props.viewerService.whenReady();
    const g = props.viewerService.getSceneGraph();
    graph.value = (g as SceneGraph) ?? null;
    if (graph.value?.rootNodeId) {
      expanded.value.add(graph.value.rootNodeId);
    }
  } finally {
    isLoading.value = false;
  }
};

const toggleExpanded = (id: string) => {
  if (expanded.value.has(id)) {
    expanded.value.delete(id);
  } else {
    expanded.value.add(id);
  }
};

const selectAndFit = (id: string) => {
  props.viewerService.selectObjectById(id);
  props.viewerService.fitAll(true);
};

const isolate = (id: string) => {
  if (!graph.value) return;
  const ids = collectSubtreeObjectIds(graph.value, id);
  props.viewerService.isolateObjectIds(ids);
};

const showAll = () => props.viewerService.showAllObjects();
const hideNode = (id: string) => props.viewerService.setObjectVisible(id, false);

const collectSubtreeObjectIds = (g: SceneGraph, rootId: string): string[] => {
  const byId = new Map<string, SceneGraphNode>();
  g.nodes.forEach(n => byId.set(n.id, n));
  const res: string[] = [];
  const stack: string[] = [rootId];
  while (stack.length) {
    const cur = stack.pop()!;
    res.push(cur);
    const node = byId.get(cur);
    if (node?.children?.length) {
      for (const childId of node.children) stack.push(childId);
    }
  }
  return res;
};

onMounted(() => {
  void refresh();
});
</script>

<template>
  <v-card variant="tonal">
    <v-card-title class="text-subtitle-2 d-flex align-center justify-space-between">
      <span>装配树</span>
      <div class="d-flex ga-2">
        <v-btn size="x-small" variant="outlined" :loading="isLoading" @click="refresh">刷新</v-btn>
        <v-btn size="x-small" variant="outlined" @click="showAll">显示全部</v-btn>
      </div>
    </v-card-title>

    <v-card-text>
      <v-text-field
        v-model="search"
        density="compact"
        variant="outlined"
        label="搜索（名称 / ID / 类型）"
        hide-details
      />

      <div v-if="!graph" class="text-caption mt-3">
        SceneGraph 暂不可用：需要重新构建/安装 wasm32 产物，并确保 TS API 已包含 `interactiveContext.getSceneGraph()`。
      </div>

      <v-list v-else density="compact" class="mt-2">
        <v-list-item
          v-for="row in filteredFlatRows"
          :key="row.id"
          :style="{ paddingLeft: `${12 + row.depth * 16}px` }"
        >
          <template #prepend>
            <v-btn
              v-if="row.hasChildren"
              icon
              size="x-small"
              variant="text"
              @click.stop="toggleExpanded(row.id)"
            >
              <v-icon>{{ row.isExpanded ? 'mdi-chevron-down' : 'mdi-chevron-right' }}</v-icon>
            </v-btn>
            <div v-else style="width: 28px;"></div>
          </template>

          <v-list-item-title class="text-body-2">
            {{ row.node.name || row.node.id }}
          </v-list-item-title>
          <v-list-item-subtitle class="text-caption">
            {{ row.node.type }} · {{ row.node.id }}
          </v-list-item-subtitle>

          <template #append>
            <v-btn icon size="x-small" variant="text" @click.stop="selectAndFit(row.id)">
              <v-icon>mdi-crosshairs-gps</v-icon>
            </v-btn>
            <v-btn icon size="x-small" variant="text" @click.stop="isolate(row.id)">
              <v-icon>mdi-eye-lock</v-icon>
            </v-btn>
            <v-btn icon size="x-small" variant="text" @click.stop="hideNode(row.id)">
              <v-icon>mdi-eye-off</v-icon>
            </v-btn>
          </template>
        </v-list-item>
      </v-list>
    </v-card-text>
  </v-card>
</template>

