const fs = require('fs');
const path = require('path');

const basePath = 'C:/Users/rober/OneDrive/Documenten/GitHub/DatzBergamo';

function readASC(filepath) {
    console.log(`Reading ${filepath}...`);
    const content = fs.readFileSync(filepath, 'utf8');
    const lines = content.trim().split('\n');

    // Parse header
    const header = {};
    for (let i = 0; i < 6; i++) {
        const parts = lines[i].trim().split(/\s+/);
        const key = parts[0];
        const value = parts[1];
        if (key === 'ncols' || key === 'nrows') {
            header[key] = parseInt(value);
        } else {
            header[key] = parseFloat(value);
        }
    }

    console.log(`  Size: ${header.ncols} x ${header.nrows} cells`);
    console.log(`  Cell size: ${header.cellsize}m`);

    // Parse data
    const data = [];
    for (let i = 6; i < lines.length; i++) {
        if (lines[i].trim()) {
            const row = lines[i].trim().split(/\s+/).map(v => parseFloat(v));
            data.push(row);
        }
    }

    return { header, data };
}

function mergeTerrains() {
    // Read both terrains
    const terrain1 = readASC(path.join(basePath, 'HamilIsland.asc'));
    const terrain2 = readASC(path.join(basePath, 'Woordbury.asc'));

    const h1 = terrain1.header;
    const h2 = terrain2.header;
    const data1 = terrain1.data;
    const data2 = terrain2.data;

    // Verify cell size matches
    if (h1.cellsize !== h2.cellsize) {
        throw new Error('Cell sizes must match!');
    }

    const cellsize = h1.cellsize;
    const nodata = h1.nodata_value;
    const seaLevel = -8; // Match original sea level

    // Offsets in cells
    const xOffset = 3300;  // 6600m / 2m
    const yOffset = 800;   // 1600m / 2m

    // Calculate merged dimensions
    const maxCol = Math.max(h1.ncols, xOffset + h2.ncols);
    const maxRow = Math.max(h1.nrows, yOffset + h2.nrows);

    console.log(`\nMerging terrains:`);
    console.log(`  Terrain 1 (HamilIsland): ${h1.ncols} x ${h1.nrows} at (0, 0)`);
    console.log(`  Terrain 2 (Woodbury): ${h2.ncols} x ${h2.nrows} at (${xOffset}, ${yOffset})`);
    console.log(`  Combined: ${maxCol} x ${maxRow} cells`);
    console.log(`  World size: ${maxCol * cellsize}m x ${maxRow * cellsize}m`);

    // Create merged grid filled with sea level
    console.log(`\nCreating merged grid...`);
    const merged = [];
    for (let row = 0; row < maxRow; row++) {
        merged.push(new Array(maxCol).fill(seaLevel));
    }

    // Copy terrain 1 (HamilIsland)
    console.log('Copying HamilIsland...');
    for (let row = 0; row < data1.length; row++) {
        for (let col = 0; col < data1[row].length; col++) {
            if (col < maxCol && row < maxRow) {
                merged[row][col] = data1[row][col];
            }
        }
    }

    // Copy terrain 2 (Woodbury) with offset
    console.log('Copying Woodbury with offset...');
    for (let row = 0; row < data2.length; row++) {
        const targetRow = row + yOffset;
        if (targetRow < maxRow) {
            for (let col = 0; col < data2[row].length; col++) {
                const targetCol = col + xOffset;
                if (targetCol < maxCol) {
                    const height = data2[row][col];
                    // Only overwrite if terrain 2 has valid data above sea level
                    if (height !== nodata && height > seaLevel) {
                        merged[targetRow][targetCol] = height;
                    }
                }
            }
        }
    }

    // Write output
    const outputPath = path.join(basePath, 'DatZ_Combined_Terrain.asc');
    console.log(`\nWriting to ${outputPath}...`);

    const output = [];
    output.push(`ncols ${maxCol}`);
    output.push(`nrows ${maxRow}`);
    output.push(`xllcorner 0`);
    output.push(`yllcorner 0`);
    output.push(`cellsize ${cellsize}`);
    output.push(`nodata_value ${nodata}`);

    for (let row = 0; row < maxRow; row++) {
        output.push(merged[row].join(' '));
        if (row % 500 === 0) {
            console.log(`  Row ${row} / ${maxRow}`);
        }
    }

    fs.writeFileSync(outputPath, output.join('\n'));

    console.log(`\n=== Merge Complete! ===`);
    console.log(`Output: ${outputPath}`);
    console.log(`Size: ${maxCol * cellsize}m x ${maxRow * cellsize}m`);
}

mergeTerrains();
