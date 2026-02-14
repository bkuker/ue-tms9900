<script setup lang="ts">
import Terminal from './components/Terminal.vue';
import Status from './components/Status.vue';
import { ref, onMounted, watch } from 'vue';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';
import RomUpload from './components/RomUpload.vue';
import List from './components/List.vue';
import Ram from './components/Ram.vue';

const ue = ref<UeTMS990>();
const term0 = ref();
const term1 = ref();
const running = ref(false);
const list = ref<string>();
const romImage = ref<Uint8Array>();

const step = () => {
  ue.value.intEnc.enabled = false;
  ue.value.cpu.run(1, false);
  ue.value.intEnc.enabled = true;
}

const stepI = () => {
  ue.value.cpu.run(1, false);
}

function handleUpload(files) {
  console.log(files);
  romImage.value = files.rom.data;
  list.value = files.list.data;
}

function runTo(a){
  if (!running.value ){
    ue.value.cpu.setAuxBreakpoint(a);
    running.value = true;
  }
  console.log("Runto", a, a.toString(16));
}

watch(romImage, (romImage) => {
  ue.value = new UeTMS990(romImage);
  ue.value.mux0.setTerminalByteConsumer((b) => term0.value.write(b));
  ue.value.mux1.setTerminalByteConsumer((b) => term1.value.write(b));
  ue.value.cpu.reset();
  //TODO Make Interrupts a thing that resets
});

onMounted(async () => {
  const response = await fetch("serialInt.rom");
  const arrayBuffer = await response.arrayBuffer();
  romImage.value = new Uint8Array(arrayBuffer);

  list.value = await (await fetch("serialInt.lst")).text();


  while (true) {
    if (running.value && ue.value) {
      ue.value.cpu.run(2000, false);
      if ( ue.value.cpu.isStoppedAtBreakpoint() ){
        running.value = false;
      }
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
  <h1>UE TMS9900 Homebrew Emulator</h1>
  <div v-if="ue" class="ue">
      <div class="term">
        <Terminal ref="term0" @byte="(b) => ue.mux0.offerByteFromTerminal(b)"></Terminal>
        MUX0
      </div>
      <div class="term">
        <Terminal ref="term1" @byte="(b) => ue.mux1.offerByteFromTerminal(b)"></Terminal>
        MUX1
      </div>
      <Status :ue="ue"></Status>
      <div class="controls">
        <h2>Controls:</h2>
        <button @click="running = true" :disabled="running">Run</button>
        <button @click="running = false" :disabled="!running">Halt</button>
        <button @click="step" :disabled="running">Step</button>
        <button @click="stepI" :disabled="running">Step with Interrupts</button>
        <div>
          Timer: <input type="range" min="0" max="50" v-model.number="ue.timer.hz">{{ ue.timer.hz }}Hz
        </div>

        <RomUpload @files-uploaded="handleUpload" />
      </div>
      <Ram :list="list" :memory="ue.memory" />
    <List :list="list" :cpu="ue.cpu" @runTo="runTo"/>
  </div>
</template>

<style scoped>
h1 {
  margin: 0;
  padding: 20px;
}
.term {
  display: inline-block;
  margin: 10px 30px 10px 30px;
  padding: 10px;
  background-color: tan;
  border-radius: 20px;
}

button {
  margin-left: 10px;
}

.controls {
  margin: 30px;
}
</style>