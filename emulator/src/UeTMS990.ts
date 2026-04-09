import { TMS9900 } from './tms9900';
import { Memory, RAM, ROM } from './memory';
import { UeUart } from './ueuart';
import { Timer } from './timer';
import { InterruptEncoder } from './InterruptEncoder';
import mmap from '../../mmap.js';

export class UeTMS990 {

    intEnc: InterruptEncoder;
    cpu: TMS9900;
    memory: Memory;
    mux0: UeUart;
    mux1: UeUart;
    timer: Timer;
    ram : RAM;
    rom : ROM;

    constructor(romData: Uint8Array) {
        this.intEnc = new InterruptEncoder();
        this.memory = new Memory([
            this.rom = new ROM(0x0000, 4 * 1024, romData),
            this.ram = new RAM(0x1000, 12 * 1024),
            //new ROM(0x0000, 8 * 1024, romData), //TEMPORARY ROM INCREASE
            //new RAM(0x2000, 12 * 1024),
            this.mux0 = new UeUart(mmap.mux0, this.intEnc),
            this.mux1 = new UeUart(mmap.mux1, this.intEnc),
            this.timer = new Timer(mmap.timer.rst, mmap.timer.irq, this.intEnc)
        ]);
        this.cpu = new TMS9900(this.memory, this.intEnc);
    }


}