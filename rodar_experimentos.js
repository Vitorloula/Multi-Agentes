const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const SMA_EXECUTABLE = path.join('.', 'build', 'multi_agentes.exe');
const BRKGA_EXECUTABLE = path.join('.', 'build', 'total_rd_brkga_executable.exe');
const DATA_DIR = path.join('total_rd_brkga', 'data', 'edges', 'BANCO - Miscellaneous Networks');
const OUTPUT_CSV = 'resultados_trd_comparativo.csv';
const RUNS_PER_INSTANCE = 10; // Para fins acadêmicos rigorosos, recomenda-se 30

function runExperiments() {
  if (!fs.existsSync(SMA_EXECUTABLE)) {
    console.error(`Erro: O executável SMA '${SMA_EXECUTABLE}' não existe. Compile o projeto primeiro.`);
    process.exit(1);
  }

  if (!fs.existsSync(BRKGA_EXECUTABLE)) {
    console.error(`Erro: O executável BRKGA '${BRKGA_EXECUTABLE}' não existe. Compile o projeto primeiro.`);
    process.exit(1);
  }

  if (!fs.existsSync(DATA_DIR)) {
    console.error(`Erro: O diretório de instâncias '${DATA_DIR}' não foi encontrado.`);
    process.exit(1);
  }

  const instances = fs.readdirSync(DATA_DIR)
    .filter(file => file.endsWith('.txt'))
    .sort();

  console.log(`Encontradas ${instances.length} instâncias.`);
  console.log(`Iniciando experimentos comparativos (rodadas por instância: ${RUNS_PER_INSTANCE})...\n`);

  const csvLines = [[
    'Instancia', 'Vertices', 'Arestas', 'Densidade',
    'Melhor_Custo_SMA', 'Media_Custo_SMA', 'Tempo_Medio_SMA_s',
    'Melhor_Custo_BRKGA', 'Media_Custo_BRKGA', 'Tempo_Medio_BRKGA_s',
    'Validacao_SMA'
  ]];

  for (const inst of instances) {
    const instPath = path.join(DATA_DIR, inst);
    console.log(`\nTestando instância: ${inst}...`);

    // --- 1. Rodar Multi-Agent System (SMA) ---
    const smaCosts = [];
    const smaTimes = [];
    let smaValidationOk = true;
    let numVertices = 0;
    let numEdges = 0;
    let density = 0.0;
    let skipInstance = false;

    for (let r = 0; r < RUNS_PER_INSTANCE; r++) {
      try {
        const stdout = execSync(`"${SMA_EXECUTABLE}" "${instPath}"`, { encoding: 'utf8', stdio: ['pipe', 'pipe', 'ignore'] });

        if (stdout.includes("excede o limite maximo")) {
          console.log(` -> Pulando ${inst} (excede o limite maximo de vertices).`);
          skipInstance = true;
          break;
        }

        if (r === 0) {
          const vMatch = stdout.match(/com (\d+) vertices e (\d+) arestas/);
          if (vMatch) {
            numVertices = parseInt(vMatch[1], 10);
            numEdges = parseInt(vMatch[2], 10);
          }
          const dMatch = stdout.match(/densidade:\s*([\d.]+)/);
          if (dMatch) {
            density = parseFloat(dMatch[1]);
          }
        }

        const costMatch = stdout.match(/Melhor custo:\s*([\d.]+)/);
        if (costMatch) smaCosts.push(parseFloat(costMatch[1]));

        const timeMatch = stdout.match(/Tempo:\s*([\d.]+)s/);
        if (timeMatch) smaTimes.push(parseFloat(timeMatch[1]));

        if (!stdout.includes('Status: OK')) {
          smaValidationOk = false;
        }
      } catch (err) {
        console.error(`Erro ao executar SMA para ${inst} na rodada ${r + 1}:`, err.message);
        smaValidationOk = false;
        skipInstance = true;
        break;
      }
    }

    if (skipInstance || smaCosts.length === 0) {
      continue;
    }

    // --- 2. Rodar BRKGA Puro ---
    const tempCsvPath = `brkga_temp_${inst}.csv`;
    if (fs.existsSync(tempCsvPath)) {
      fs.unlinkSync(tempCsvPath);
    }

    const brkgaCosts = [];
    const brkgaTimes = [];

    try {
      // Executa o BRKGA puro para rodar os trials e gerar o CSV temporário
      execSync(`"${BRKGA_EXECUTABLE}" "${instPath}" --trials ${RUNS_PER_INSTANCE} --output "${tempCsvPath}"`, { stdio: 'ignore' });

      if (fs.existsSync(tempCsvPath)) {
        const content = fs.readFileSync(tempCsvPath, 'utf8');
        const lines = content.trim().split('\n');
        // A primeira linha é o cabeçalho
        for (let i = 1; i < lines.length; i++) {
          const parts = lines[i].split(',');
          if (parts.length >= 5) {
            // fitness_value é a quarta coluna (index 3)
            brkgaCosts.push(parseFloat(parts[3]));
            // elapsed_time(microseconds) é a quinta coluna (index 4)
            // Convertendo microsegundos para segundos
            brkgaTimes.push(parseFloat(parts[4]) / 1000000.0);
          }
        }
        fs.unlinkSync(tempCsvPath);
      }
    } catch (err) {
      console.error(`Erro ao executar BRKGA para ${inst}:`, err.message);
      if (fs.existsSync(tempCsvPath)) {
        fs.unlinkSync(tempCsvPath);
      }
    }

    if (brkgaCosts.length > 0) {
      const bestSma = Math.min(...smaCosts);
      const avgSma = smaCosts.reduce((a, b) => a + b, 0) / smaCosts.length;
      const timeSma = smaTimes.reduce((a, b) => a + b, 0) / smaTimes.length;

      const bestBrkga = Math.min(...brkgaCosts);
      const avgBrkga = brkgaCosts.reduce((a, b) => a + b, 0) / brkgaCosts.length;
      const timeBrkga = brkgaTimes.reduce((a, b) => a + b, 0) / brkgaTimes.length;

      const validationSma = smaValidationOk ? 'OK' : 'ERRO';

      csvLines.push([
        inst,
        numVertices,
        numEdges,
        density.toFixed(6),
        bestSma.toFixed(2),
        avgSma.toFixed(2),
        timeSma.toFixed(4),
        bestBrkga.toFixed(2),
        avgBrkga.toFixed(2),
        timeBrkga.toFixed(4),
        validationSma
      ]);

      console.log(` -> SMA   | Melhor: ${bestSma} | Média: ${avgSma.toFixed(2)} | Tempo: ${timeSma.toFixed(4)}s | Validação: ${validationSma}`);
      console.log(` -> BRKGA | Melhor: ${bestBrkga} | Média: ${avgBrkga.toFixed(2)} | Tempo: ${timeBrkga.toFixed(4)}s`);
    } else {
      console.warn(`Aviso: Nenhuma saída coletada do BRKGA para a instância ${inst}`);
    }
  }

  const csvContent = csvLines.map(line => line.join(',')).join('\n');
  fs.writeFileSync(OUTPUT_CSV, csvContent, 'utf8');

  console.log(`\nExperimentos concluídos com sucesso!`);
  console.log(`Resultados salvos em: ${OUTPUT_CSV}`);
}

runExperiments();
