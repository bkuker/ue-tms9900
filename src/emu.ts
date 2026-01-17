import {TMS9900} from './emulator/classes/tms9900';
import {Memory} from './emulator/classes/memory';
import {CRU} from './emulator/classes/cru';


let memory = new Memory();
let cru = new CRU();
let cpu = new TMS9900(memory, cru);

cpu.reset();
cpu.run(10000, false);