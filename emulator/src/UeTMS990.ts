import { TMS9900 } from './tms9900';
import { Memory, RAM, ROM } from './memory';
import { UeUart } from './ueuart';
import { Timer } from './timer';
import { InterruptEncoder } from './InterruptEncoder';

export class UeTMS990 {

    intEnc: InterruptEncoder;
    cpu: TMS9900;
    memory: Memory;
    mux0: UeUart;
    mux1: UeUart;
    timer: Timer;

    constructor(romData: Uint8Array) {
        this.intEnc = new InterruptEncoder();
        this.memory = new Memory([
            new ROM(0x0000, 2 * 1024, romData),
            new RAM(0x1000, 12 * 1024),
            this.mux0 = new UeUart(0xF000, this.intEnc),
            this.mux1 = new UeUart(0XF00A, this.intEnc),
            this.timer = new Timer(0xF0F0, 15, this.intEnc)
        ]);
        this.cpu = new TMS9900(this.memory, this.intEnc);
    }


}