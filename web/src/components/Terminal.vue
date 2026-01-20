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
    fontSize: 14,
    theme: {
      background: '#1e1e1e',
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

<style>
.terminal-container {
  width: fit-content;  /* Shrink to terminal's actual size */
  height: 400px;       /* Or whatever height you want */
  display: inline-block;
}
</style>