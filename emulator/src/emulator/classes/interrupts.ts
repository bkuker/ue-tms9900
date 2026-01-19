/**
 * The devices on the bus have open collectors on IC0-3 wire anding them together.
 * Each emulated device registers as an InterruptSource, providing it's own IC0-3
 * values and IntReq value. This code combines them together and presents a single
 * value to the CPU.
 * 
 * I think this could cause oddness if multiple interrupts occur at the same time
 * but this matches the current hardware
 * 
 */

export interface InterruptSource {
    getInterruptCode(): number;  // Returns 0-15, or 15 if not asserting
    isAssertingInterrupt(): boolean;
}

const devices: InterruptSource[] = [];

export function registerInterruptSource(device: InterruptSource) {
    devices.push(device);
}

export function getInterruptState(): { intReq: boolean, ic: number } {
    // INTREQ is ORed - true if ANY device is asserting
    const intReq = devices.some(d => d.isAssertingInterrupt());

    if (!intReq) {
        return { intReq: false, ic: 15 };  // No interrupt
    }

    // IC0-IC3 are open collector - wire-AND of all outputs
    // Start with all bits high (0xF = 1111)
    let ic = 0xF;

    for (const device of devices) {
        if (device.isAssertingInterrupt()) {
            // Bitwise AND - any device pulling low wins
            ic &= device.getInterruptCode();
        }
    }

    let ret = { intReq: true, ic };
    //console.log(ret);
    return ret;
}