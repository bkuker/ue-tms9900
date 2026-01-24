<script setup lang="ts">
import Terminal from './components/Terminal.vue';
import Status from './components/Status.vue';
import { ref, onMounted } from 'vue';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';

const ue = ref();
const term0 = ref();
const term1 = ref();
const running = ref(true);


const step = () => {
  ue.value.cpu.run(1, false);
}

onMounted(async () => {
  const response = await fetch("hellorld2.rom");
  const arrayBuffer = await response.arrayBuffer();
  const romContents = new Uint8Array(arrayBuffer);

  ue.value = new UeTMS990(romContents);

  ue.value.mux0.setTerminalByteConsumer((b)=>term0.value.write(b));
  ue.value.mux1.setTerminalByteConsumer((b)=>term1.value.write(b));
  ue.value.cpu.reset();

  while (true) {
    if (running.value) {
      ue.value.cpu.run(2000, false);
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
  <div v-if="ue">
    <div class="term">
      <Terminal ref="term0" @byte="(b) => ue.mux0.offerByteFromTerminal(b)"></Terminal>
      MUX0
    </div>
    <div class="term">
      <Terminal ref="term1" @byte="(b) => ue.mux1.offerByteFromTerminal(b)"></Terminal>
      MUX1
    </div>
    <Status :cpu="ue.cpu"></Status>
    Controls:
    <button @click="running = true" :disabled="running">Run</button>
    <button @click="running = false" :disabled="!running">Halt</button>
    <button @click="step" :disabled="running">Step</button>
    <div>
      Timer: <input type="range" min="0" max="50" v-model.number="ue.timer.hz">{{ ue.timer.hz }}Hz
    </div>
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