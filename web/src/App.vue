<script setup lang="ts">
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { Memory } from '@ue-tms9900/emulator/memory';
import Terminal from './components/Terminal.vue';
import { ref, onMounted } from 'vue';

const mux0 = ref();
const mux1 = ref();

const term0 = ref();
const term1 = ref();

onMounted(async () => {
  const response = await fetch("hellorld.rom");
  const arrayBuffer = await response.arrayBuffer();
  const romContents = new Uint8Array(arrayBuffer);

  let memory = new Memory(romContents);

  mux0.value = memory.mux0;
  memory.mux0.setTerminalByteConsumer(term0.value.write);

  mux1.value = memory.mux1;
  memory.mux1.setTerminalByteConsumer(term1.value.write);

  let cpu = new TMS9900(memory);
  cpu.reset();

  while (true) {
    cpu.run(1000, false);
    //But yield every so often so UARTs and other things 
    //do their thing
    await new Promise(r => setTimeout(r, 0));
  }

});
</script>

<template>
  MUX0:
  <Terminal ref="term0" @byte="(b) => mux0.offerByteFromTerminal(b)"></Terminal>
  MUX1:
  <Terminal ref="term1" @byte="(b) => mux1.offerByteFromTerminal(b)"></Terminal>
</template>

<style scoped></style>