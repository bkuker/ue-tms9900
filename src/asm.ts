import path from "path";
import os from "os";
import fs from "fs";
import { spawn } from "child_process";
import { Command } from 'commander';


const ASM_CMD = `${process.cwd()}/Assembly/TI990 Tools/asm990/asm990.exe`;
const LINK_CMD = `${process.cwd()}/Assembly/TI990 Tools/lnk990/lnk990.exe`;

//If running from "npm run" change back
//to the directory the user ran the program from
if (process.env["INIT_CWD"]) {
    process.chdir(process.env["INIT_CWD"]);
}

//Get Command Line
const commandLine = new Command();
commandLine
    .argument('<assembly file name>');
commandLine.parse();


main().catch((err) => {
    console.error(err);
    process.exit(1);
});



async function main(): Promise<void> {
    const fileName = commandLine.args[0];
    if (!fileName.endsWith(".asm"))
        throw new Error("Expected .asm File");

    const dir = path.dirname(fileName);
    const name = path.parse(fileName).name;
    const baseName = path.join(dir, name);
    const lst = `${baseName}.lst`;
    const obj = `${baseName}.obj`;
    const aout = `${baseName}.aout`;

    if ( true ){
        await run(ASM_CMD, ["-l", lst, "-o", obj, fileName]);
        const text = fs.readFileSync(obj, "utf-8");
        const unixText = text.replace(/\r\n/g, "\n");
        fs.writeFileSync(obj, unixText, "utf-8");
    } else {
        await run(ASM_CMD, ["-c", "-l", lst, "-o", obj, fileName]);
    }

    await run(LINK_CMD, ["-a", "-o", aout, obj]);

    const romA = fs.openSync(`${baseName}.a.rom`, "w");
    const romB = fs.openSync(`${baseName}.b.rom`, "w");
    const rom = fs.openSync(`${baseName}.rom`, "w");

    const bin = fs.openSync(aout, "r");
    const buffer = Buffer.alloc(1);
    let position = 0;
    while (fs.readSync(bin, buffer, 0, 1, null) > 0) {
        if (position >= 16) {
            fs.writeSync(rom, buffer, 0, 1);
            if (position % 2 == 0)
                fs.writeSync(romA, buffer, 0, 1);
            else
                fs.writeSync(romB, buffer, 0, 1);
        }
        position++;
    }

    fs.closeSync(romA);
    fs.closeSync(romB);
    fs.closeSync(rom);
}



export function run(
    exe: string,
    args: string[] = []
): Promise<string> {
    return new Promise((resolve, reject) => {
        const child = spawn(exe, args, {
            shell: false,          // IMPORTANT on Windows
            windowsHide: true
        });

        let output = "";
        let errorOutput = "";

        child.stdout.on("data", (d) => output += d.toString());
        child.stderr.on("data", (d) => errorOutput += d.toString());

        child.on("error", () => reject(new Error(`STDOUT:\n${output}\n\nSTDERR:\n${errorOutput}`)));

        child.on("close", (code) => {
            if (code === 0) resolve(output);
            else reject(new Error(`STDOUT:\n${output}\n\nSTDERR:\n${errorOutput}\n\nExit code ${code}`));
        });
    });
}