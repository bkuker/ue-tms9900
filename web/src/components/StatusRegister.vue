<template>
  <div class="status-register">
    <span :class="{ set: lgt }">LGT</span>
    <span :class="{ set: agt }">AGT</span>
    <span :class="{ set: eq }">EQ</span>
    <span :class="{ set: c }">C</span>
    <span :class="{ set: ov }">OV</span>
    <span :class="{ set: op }">OP</span>
    <span :class="{ set: x }">X</span>
    <span class="int-mask">INT:{{ interruptMask }}</span>
  </div>
</template>

<script setup>
import { computed } from 'vue';

const props = defineProps({
  modelValue: {
    type: Number,
    default: 0
  }
});

// Extract status bits from the 16-bit status word
// TMS9900 Status Register layout:
// Bits 0-3: Interrupt mask
// Bit 12: L (Logical greater than)
// Bit 13: A (Arithmetic greater than)  
// Bit 14: E (Equal)
// Bit 15: C (Carry)

const interruptMask = computed(() => props.modelValue & 0x000F);
const lgt = computed(() => (props.modelValue & 0x8000) !== 0); // Bit 15
const agt = computed(() => (props.modelValue & 0x4000) !== 0); // Bit 14
const eq = computed(() => (props.modelValue & 0x2000) !== 0);  // Bit 13
const c = computed(() => (props.modelValue & 0x1000) !== 0);   // Bit 12
const ov = computed(() => (props.modelValue & 0x0800) !== 0);  // Bit 11
const op = computed(() => (props.modelValue & 0x0400) !== 0);  // Bit 10
const x = computed(() => (props.modelValue & 0x0200) !== 0);   // Bit 9
</script>

<style scoped>
.status-register {
  font-family: monospace;
  display: inline-block;
}

.status-register span {
  margin-right: 0.5rem;
}

.status-register span.set {
  color: #10b981;
  font-weight: bold;
}

.int-mask {
  color: #22d3ee;
  font-weight: bold;
}
</style>