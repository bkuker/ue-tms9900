import * as fs from 'fs';
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';
import { Command } from 'commander';

//Get Command Line
const commandLine = new Command();
commandLine
    .argument('<rom file name>');
commandLine.parse();

main().catch((err) => {
    console.error(err);
    process.exit(1);
});


async function main(): Promise<void> {
    const rom = commandLine.args[0]
    console.log(`Loading ${rom} @0x0000`);
    const buffer = fs.readFileSync(rom);
    let romContents = new Uint8Array(buffer);

    let ue = new UeTMS990(romContents);

    let buf: number[] = [];
    process.stdin.setRawMode(true);
    process.stdin.resume();
    process.stdin.setEncoding('utf8');
    process.stdin.on("data", (key: string) => {
        if (key === "\u0003") {
            console.log("PC:", ue.cpu.getPc().toString(16));
            process.exit(); // Ctrl+C
        } else {
            for (let c of key) {
                buf.push(c.charCodeAt(0));    //Append to buffer
            }
        }
    });
    ue.mux0.setTerminalByteConsumer(b => process.stdout.write(`\x1b[32m${String.fromCharCode(b)}\x1b[0m`));
    ue.mux1.setTerminalByteConsumer(b => process.stdout.write(`\x1b[33m${String.fromCharCode(b)}\x1b[0m`));

    ue.cpu.reset();

    //Run the CPU Continuously...
    while (true) {
        try {
            ue.cpu.run(1000, false);
            if (buf.length) {
                if (ue.mux0.offerByteFromTerminal(buf[0])) {
                    buf.shift();
                }
            }
        } catch (e) {
            //console.log(e);
            await new Promise(r => setImmediate(r));
            break;
        }
        //But yeild every so often so UARTs and other things 
        //do their thing
        await new Promise(r => setImmediate(r));
    }
}