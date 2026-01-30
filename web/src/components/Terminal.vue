<template>
  <div ref="terminalRef" class="terminal-container"></div>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import { Terminal } from '@xterm/xterm'
import '@xterm/xterm/css/xterm.css'

const terminalRef = ref(null)
let term = null

// Define the emit
const emit = defineEmits(['byte', 'keystroke'])

onMounted(() => {
  term = new Terminal({
    cols: 80,
    rows: 24,
    cursorBlink: true,
    fontSize: 12,
    theme: {
      background: '#252525',
      foreground: '#d4d4d4'
    }
  })

  term.open(terminalRef.value)

  // Emit on each keystroke
  term.onData(data => {
    emit('byte', data.charCodeAt(0))
  })

  // Optional: also emit on key events for more detail
  term.onKey(({ key, domEvent }) => {
    // This gives you the actual key and DOM event
    emit('keystroke', key)
  })
})

onUnmounted(() => {
  term?.dispose()
})

// Method to write output to terminal
const write = (byte) => {
  term?.write(String.fromCharCode(byte));
}

defineExpose({ write })
</script>

<style scoped>
.terminal-container {
  width: fit-content;
  /* Shrink to terminal's actual size */

  border: 20px solid #4A392E;
  border-radius: 10px;
  padding: 10px 0 10px 10px;
  background-color: #252525;
}

.xterm-viewport {}
</style>