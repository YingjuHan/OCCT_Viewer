import { ref } from 'vue';
import type { LocalLoadResult } from '../services/viewer.service';

const MAX_LOG_ITEMS = 6;

export function useViewerLogs() {
  const loadLog = ref<LocalLoadResult[]>([]);

  const handleFilesLoaded = (results: LocalLoadResult[]): void => {
    if (!results || results.length === 0) {
      return;
    }
    loadLog.value = [...results, ...loadLog.value].slice(0, MAX_LOG_ITEMS);
  };

  return {
    loadLog,
    handleFilesLoaded
  };
}
