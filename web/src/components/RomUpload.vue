<template>
    <div class="file-uploader">
        <div class="upload-area">
            <label for="files" class="file-label">ROM Upload: </label>
            <input id="files" type="file" multiple accept=".rom,.lst" @change="handleFiles" class="file-input" />

            <div v-if="selectedFiles.length > 0" class="selected-files">
                <ul>
                    <li v-for="file in selectedFiles" :key="file.name">
                        {{ file.name }} <span class="file-type">({{ file.type || getFileType(file.name) }})</span>
                    </li>
                </ul>
            </div>

            <button v-if="canUpload" @click="uploadFiles" class="upload-button">
                Load & Reset
            </button>

            <p v-if="error" class="error">{{ error }}</p>
        </div>
    </div>
</template>

<script setup>
import { ref, computed } from 'vue';

const emit = defineEmits(['filesUploaded']);

const selectedFiles = ref([]);
const error = ref('');

const canUpload = computed(() => {
    const romCount = selectedFiles.value.filter(f => f.name.endsWith('.rom')).length;
    const lstCount = selectedFiles.value.filter(f => f.name.endsWith('.lst')).length;
    return romCount === 1 && lstCount <= 1;
});

const handleFiles = (event) => {
    selectedFiles.value = Array.from(event.target.files);
    error.value = '';

    const romCount = selectedFiles.value.filter(f => f.name.endsWith('.rom')).length;
    const lstCount = selectedFiles.value.filter(f => f.name.endsWith('.lst')).length;

    if (romCount !== 1) {
        error.value = 'Please select exactly one .rom file';
    } else if (lstCount > 1) {
        error.value = 'Please select at most one .lst file';
    }
};

const getFileType = (filename) => {
    return filename.split('.').pop();
};

const uploadFiles = async () => {
    if (!canUpload.value) return;

    try {
        const romFile = selectedFiles.value.find(f => f.name.endsWith('.rom'));
        const listFile = selectedFiles.value.find(f => f.name.endsWith('.lst'));

        // Read ROM file as ArrayBuffer
        const romData = await readFileAsArrayBuffer(romFile);

        // Read LIST file as text
        const listData = await readFileAsText(listFile);

        // Emit both files to parent component
        emit('filesUploaded', {
            rom: {
                name: romFile.name,
                data: romData
            },
            list: {
                name: listFile.name,
                data: listData
            }
        });

        // Reset
        selectedFiles.value = [];
        error.value = '';
    } catch (err) {
        error.value = 'Error reading files: ' + err.message;
        console.error('Error reading files:', err);
    }
};

const readFileAsArrayBuffer = (file) => {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => resolve(reader.result);
        reader.onerror = reject;
        reader.readAsArrayBuffer(file);
    });
};

const readFileAsText = (file) => {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => resolve(reader.result);
        reader.onerror = reject;
        reader.readAsText(file);
    });
};
</script>

<style scoped>

.file-type {
    color: #6b7280;
    font-size: 0.75rem;
}

.error {
    color: #dc2626;
    font-size: 0.875rem;
    margin: 0;
}
</style>