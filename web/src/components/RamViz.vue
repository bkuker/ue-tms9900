<template>
  <div style="position: relative; display: inline-block">
    <canvas ref="canvas" :width="width" :height="height" @mousemove="onMouseMove"
      @mouseleave="tooltip.visible = false" />
    <div v-if="tooltip.visible" :style="{
      position: 'fixed',
      left: tooltip.x + 10 + 'px',
      top: tooltip.y + 10 + 'px',
      background: 'black',
      color: 'lime',
      fontFamily: 'monospace',
      fontSize: '12px',
      padding: '2px 6px',
      pointerEvents: 'none',
    }">
      {{ tooltip.text }}
    </div>
  </div>
</template>

<script setup>
import { TMS9900 } from '@ue-tms9900/emulator/tms9900'
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990'
import { ref, reactive, onMounted, onUnmounted, defineProps } from 'vue'

const props = defineProps({
  width: { type: Number, default: 64 },
  height: { type: Number, default: 256 },
  ue: UeTMS990
});

const canvas = ref(null);
const tooltip = reactive({ visible: false, x: 0, y: 0, text: '' });
let ctx = null;
let rafId = null;
const rgba = new Uint32Array(props.width * props.height);

function onMouseMove(e) {
  const rect = canvas.value.getBoundingClientRect()
  const x = Math.floor((e.clientX - rect.left) / rect.width * props.width);
  const y = Math.floor((e.clientY - rect.top) / rect.height * props.height);
  const addr = (y * props.width + x);;
  tooltip.visible = true;
  tooltip.x = e.clientX;
  tooltip.y = e.clientY;
  let addrTxt = `0x${addr.toString(16).toUpperCase().padStart(4, '0')}  [${addr}]`;
  let valTxt;
  if (addr >= 4096)
    valTxt = `0x${props.ue.ram.data[addr - 4096].toString(16)}`;
  else
    valTxt = `0x${props.ue.rom.data[addr].toString(16)}`;
  tooltip.text = `${addrTxt} = ${valTxt}`;
}

function render() {
  const rom = props.ue.rom.data;
  for (let i = 0; i < rom.length; i++) {
    const v = rom[i];
    rgba[i] = (255 << 24) | (v << 16) | (v << 8) | v;
  }

  const ram = props.ue.ram.data;
  for (let i = 0; i < ram.length; i++) {
    const v = ram[i];
    rgba[i + 4096] = (255 << 24) | (v << 16) | (v << 8) | v;
  }
  rgba[props.ue.cpu.getPc()] = (255 << 24) | (0 << 16) | (0 << 8) | 255;
  const imageData = new ImageData(new Uint8ClampedArray(rgba.buffer), props.width, props.height);
  ctx.putImageData(imageData, 0, 0);
  rafId = requestAnimationFrame(render);
}

onMounted(() => {
  ctx = canvas.value.getContext('2d');
  rafId = requestAnimationFrame(render);
})

onUnmounted(() => {
  cancelAnimationFrame(rafId);
})
</script>

<style scoped>
canvas {
  height: 768px;
  image-rendering: pixelated;
}
</style>