<script setup lang="ts">
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { Memory } from '@ue-tms9900/emulator/memory';
import Terminal from './components/Terminal.vue';
import Status from './components/Status.vue';
import { ref, onMounted } from 'vue';

const mux0 = ref();
const mux1 = ref();

const term0 = ref();
const term1 = ref();

const cpu = ref();
const timer = ref();

const running = ref(true);


const step = () => {
  cpu.value.run(1, false);
}

onMounted(async () => {
  const response = await fetch("hellorld2.rom");
  const arrayBuffer = await response.arrayBuffer();
  const romContents = new Uint8Array(arrayBuffer);



  let memory = new Memory(romContents);

  timer.value = memory.timer;

  mux0.value = memory.mux0;
  memory.mux0.setTerminalByteConsumer(term0.value.write);

  mux1.value = memory.mux1;
  memory.mux1.setTerminalByteConsumer(term1.value.write);

  cpu.value = new TMS9900(memory);
  cpu.value.reset();

  while (true) {
    if (running.value) {
      cpu.value.run(2000, false);
      //But yield every so often so UARTs and other things 
      //do their thing
      await new Promise(r => setTimeout(r, 0));
    } else {
      await new Promise(r => setTimeout(r, 100));
    }
  }

});
</script>

<template>
  <h1>UE TMS990 Homebrew Emulator</h1>
  <div class="term">
    <Terminal ref="term0" @byte="(b) => mux0.offerByteFromTerminal(b)"></Terminal>
    MUX0
  </div>
  <div class="term">
    <Terminal ref="term1" @byte="(b) => mux1.offerByteFromTerminal(b)"></Terminal>
    MUX1
  </div>
  <Status :cpu="cpu"></Status>
  Controls:
  <button @click="running = true" :disabled="running">Run</button>
  <button @click="running = false" :disabled="!running">Halt</button>
  <button @click="step" :disabled="running">Step</button>
  <div v-if="timer">
    Timer: <input type="range" min="0" max="50" v-model.number="timer.hz">{{ timer.hz }}Hz
  </div>
</template>

<style scoped>
.term {
  display: inline-block;
  margin: 10px 30px 10px 30px;
  padding: 10px;
  background-color: tan;
  border-radius: 30px;
}
</style>