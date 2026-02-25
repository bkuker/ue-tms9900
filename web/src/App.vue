<script setup lang="ts">
import Terminal from './components/Terminal.vue';
import Status from './components/Status.vue';
import { markRaw, ref, onMounted, watch } from 'vue';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';
import RomUpload from './components/RomUpload.vue';
import List from './components/List.vue';
import History from './components/History.vue';
import RamViz from './components/RamViz.vue';

const ue = ref<UeTMS990>();
const term0 = ref();
const term1 = ref();
const running = ref(false);
const fast = ref(false);
const list = ref<string>();
const romImage = ref<Uint8Array>();

const cyclesPerLoop = ref(50000);

const listRef = ref();

  let buf0 : number[] = [];
  let buf1 : number[] = [];

const step = () => {
  ue.value.intEnc.enabled = false;
  ue.value.cpu.run(1, false);
  ue.value.intEnc.enabled = true;
  listRef.value?.scrollToPC();
}

const stepI = () => {
  ue.value.cpu.run(1, false);
}

function handleUpload(files) {
  console.log(files);
  romImage.value = files.rom.data;
  list.value = files.list.data;
}

function runTo(a) {
  if (!running.value) {
    ue.value.cpu.setAuxBreakpoint(a);
    running.value = true;
  }
  console.log("Runto", a, a.toString(16));
}

watch(romImage, (romImage) => {
  ue.value = new UeTMS990(romImage);
  markRaw(ue.value);
    markRaw(ue.value.memory);
      markRaw(ue.value.cpu);
  markRaw(ue.value.ram);
  markRaw(ue.value.ram.data);
  markRaw(ue.value.rom);
  markRaw(ue.value.rom.data);
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
      ue.value.cpu.run(cyclesPerLoop.value, false);
      if (ue.value.cpu.isStoppedAtBreakpoint()) {
        running.value = false;
      }
      //But yield every so often so UARTs and other things 
      //do their thing
      await new Promise(r => setTimeout(r, 0));
      if ( buf0.length && ue.value.mux0.offerByteFromTerminal(buf0[0]) ){
        buf0.shift();
      }
      if ( buf1.length && ue.value.mux1.offerByteFromTerminal(buf1[0]) ){
        buf1.shift();
      }
      if (!running.value) {
        listRef.value?.scrollToPC();
      }
    } else {
      await new Promise(r => setTimeout(r, 100));
    }
  }

});
</script>

<template>
  <div class="emulator" v-if="ue">
    <div class="term0 term">
      <Terminal ref="term0" @byte="(b) => buf0.push(b)"></Terminal>
    </div>
    <div class="term1 term">
      <Terminal ref="term1" @byte="(b) => buf1.push(b)"></Terminal>
    </div>
    <div class="status">
      <Status  :ue="ue"></Status>
    </div>
    <div class="controls">
      <h2>Controls:</h2>
      <button @click="running = true" :disabled="running">Run</button>
      <button @click="running = false" :disabled="!running">Halt</button>
      <button @click="step" :disabled="running">Step</button>
      <button @click="stepI" :disabled="running">Step with Interrupts</button>
      <div>
        Timer: <input type="range" min="0" max="50" v-model.number="ue.timer.hz">{{ ue.timer.hz }}Hz
      </div>
      <div>
        Cycles Per Loop: <input type="range" min="1" max="100000" v-model.number="cyclesPerLoop">{{ cyclesPerLoop }}Hz
      </div>
      <RomUpload @files-uploaded="handleUpload" />
    </div>
    <div class="list">
      <List v-if="!running || !fast" :list="list" :cpu="ue.cpu" @runTo="runTo" ref="listRef" />
    </div>
    <div class="history">
      <!--<History v-if="!running || !fast" :cpu="ue.cpu" />-->
      <RamViz v-if="!fast" :ue="ue"/>
    </div>
  </div>
</template>

<style scoped>
.emulator {
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  display: grid;
  grid-template-areas:
    "term0 term1 list"
    "status history list"
    "controls history list";
  grid-template-columns: 1fr 1fr 1fr;
  grid-template-rows: auto 1fr 1fr;
}

.emulator > div {
  border: 1px solid black;
}

div.term0 {
  grid-area: term0;
}

div.term1 {
  grid-area: term1;
}

div.status {
  grid-area: status;
  padding: 20px;
}

div.controls {
  grid-area: controls;
  padding: 20px;
}

div.list {
  grid-area: list;
  overflow-y: scroll;
}
.history {
  grid-area: history;
  overflow-y: scroll;
}

h1 {
  margin: 0;
  padding: 20px;
}

.term {
  margin: 3px;
}

.emulator.stylish .term {
  display: inline-block;
  margin: 5px 10px 5px 10px;
  padding: 10px;
  background-color: tan;
  border-radius: 10px;
}

button {
  margin-left: 10px;
}
</style>