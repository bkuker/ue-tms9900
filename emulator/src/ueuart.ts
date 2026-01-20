/**
 * Emulates the UART card from the UE Homebrew TMS990 computer
 * 
 * 
 * THRE    EQU >F000 <- Read, byte 1 means ready to transmit
 * THRL    EQU >F002 -> Write data to send
 * RRD     EQU >F004 <- Read data received
 * DRR     EQU >F006 -> Write to reset data received signal
 */

import { Log } from './util/log';
import { InterruptSource, registerInterruptSource } from './interrupts';
import { MemoryMapped } from './memory';

export class UeUart implements InterruptSource, MemoryMapped {
    private log: Log = Log.getLog();

    private rData: number;
    private rReady: boolean;

    private tData: number;
    private tReady: boolean;

    private baseAddr: number;

    private byteConsumer: (byte: number) => void;

    constructor(baseAddr: number) {
        this.baseAddr = baseAddr;
        this.rData = 64;
        this.rReady = false;
        this.tData = 0;
        this.tReady = true;

        registerInterruptSource(this);

        this.byteConsumer = (b) => console.log(`Serial byte ${b}`);
        /*
        process.stdin.setRawMode(true);
        process.stdin.resume();
        process.stdin.setEncoding('utf8');

        process.stdin.on("data", (key: string) => {
            if (key === "\u0003")
                process.exit(); // Ctrl+C
            this.rData = key.charCodeAt(0);
            this.rReady = true;
        });*/
    }

    public offerByteFromTerminal(byte: number): boolean {
        if (this.rReady)
            return false;
        this.rData = byte;
        this.rReady = true;
        return true;
    }

    public setTerminalByteConsumer(f: (byte: number) => void): void {
        this.byteConsumer = f;
    }

    public getBaseAddress() {
        return this.baseAddr;
    };

    public getSize() {
        return 10;
    }

    getInterruptCode(): number {
        return 8;
    }
    isAssertingInterrupt(): boolean {
        return this.rReady;
    }

    public readWord(addr: number): number {
        if (addr == this.baseAddr) {
            //THRE Transmitter Holding Register Empty
            //  Is it free to accept another byte to send?
            return this.tReady ? 0 : 1;
        } else if (addr == this.baseAddr + 4) {
            //RRD Read the data
            if (!this.rReady)
                this.log.warn("UART read when no data ready")
            //console.log("UART READ " + this.rData);
            return this.rData;
        } else {
            //Not weird for RMW for MOVB
            //this.log.warn("Weird UART read from address 0x" + addr.toString(16));
        }
    }

    public writeWord(addr: number, w: number) {
        if (addr == this.baseAddr + 2) {
            //THRL, load data to send
            this.tData = w & 0xFF;
            //console.log("UART Write 0x" +this.tData.toString(16) + " " + String.fromCharCode(this.tData));
            setTimeout(() => {
                this.byteConsumer(this.tData);
                this.tReady = true;
            }, 4);
            this.tReady = false;
        } else if (addr == this.baseAddr + 6) {
            //DRR, reset data ready
            //console.log("Clear DDR")
            this.rReady = false;
        } else {
            this.log.warn("Weird UART write " + w + " to address 0x" + addr.toString(16));
        }
    }

}