import { CPU } from './interfaces/cpu';
import { InterruptSource, registerInterruptSource } from './interrupts';
import { MemoryMapped } from './memory';

const MIN_MS = 10;

export class Timer implements InterruptSource, MemoryMapped {
    private baseAddr: number;
    private intReq: boolean = false;
    private irq: number;

    private hertz: number;
    private interval: number = 0;

    constructor(baseAddr, irq) {
        this.baseAddr = baseAddr;
        this.irq = irq;
        registerInterruptSource(this);
        this.hz = 0;
    }

    //Timer Interrupt
    public set hz(hertz: number) {
        clearInterval(this.interval);
        if (hertz) {
            this.hertz = hertz;
            let interval = this.interval = setInterval(() => this.intReq = true, Math.max(MIN_MS, Math.round(1000 / hertz)));
        } else {
            this.hertz = 0;
        }
    }

    public get hz() {
        return this.hertz;
    }

    getInterruptCode(): number {
        return this.irq;
    }

    isAssertingInterrupt(): boolean {
        return this.intReq;
    }

    getBaseAddress(): number {
        return this.baseAddr;
    }

    getSize(): number {
        return 2;
    }

    readWord(addr: number, cpu: CPU | null): number {
        return this.intReq ? 1 : 0;
    }

    writeWord(addr: number, w: number, cpu: CPU) {
        this.intReq = false;
    }

}