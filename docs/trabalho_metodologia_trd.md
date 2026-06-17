# Guia Científico e Metodológico para Trabalho Acadêmico (TRD)

Este documento descreve as diretrizes metodológicas, o funcionamento do validador integrado e um roteiro prático para realizar testes, coletar dados e estruturar a seção de experimentos para um artigo ou trabalho de conclusão sobre o problema de **Dominação Romana Total (TRD)** no sistema multiagentes cooperativos.

---

## 1. O Validador de Restrições Integrado

Como parte deste plano, foi integrada uma rotina de validação formal independente no arquivo [trd.cpp](file:///d:/Multi%20Agentes/src/core/trd/trd.cpp) (`trd_validate_solution`). Após o término da busca de cada instância, o sistema analisa a melhor solução do Blackboard e verifica duas condições essenciais para a corretude da Dominação Romana Total:

1. **Dominação por Vizinhos com Peso 2**: Todo vértice $u$ cuja solução atribuiu rótulo $0$ (não ocupado) deve ter pelo menos um vizinho $v$ na rede com rótulo $2$ (duas unidades militares).
2. **Conectividade Total (Sem Vértices Isolados)**: Todo vértice ativo (com peso $1$ ou $2$) deve ter pelo menos um vizinho ativo (rótulo $\ge 1$) para garantir a resposta rápida/comunicação entre as guarnições militares.

Se ambas as regras forem obedecidas, o console exibirá:
`[VALIDACAO TRD] Status: OK (Solucao respeita todas as restricoes!)`

Caso contrário, o validador reporta o erro e o vértice culpado, assegurando que você não publique resultados inválidos em seu trabalho.

---

## 2. Roteiro Passo a Passo para Coleta de Experimentos

Para validar cientificamente sua abordagem, você deve comparar o desempenho do **Sistema Multiagentes Cooperativo** com a abordagem heurística tradicional ou com o **BRKGA puro** (da pasta `total_### Passo A: Criar um Script de Automação de Testes
Para não rodar cada instância manualmente, criamos um script em Node.js (`rodar_experimentos.js`) que percorre os arquivos de grafos da pasta `total_rd_brkga/data/edges/`, executa o binário do multiagente para cada um deles (por exemplo, 10 ou 30 rodadas por instância com sementes diferentes) e salva os resultados em uma planilha `.csv`.

Você pode salvar o script abaixo como `rodar_experimentos.js` na raiz do seu workspace:

```javascript
const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// Configurações do experimento
const EXECUTABLE_PATH = path.join('.', 'build', 'multi_agentes.exe');
const DATA_DIR = path.join('total_rd_brkga', 'data', 'edges', 'BANCO - Miscellaneous Networks');
const OUTPUT_CSV = 'resultados_trd_multiagentes.csv';
const RUNS_PER_INSTANCE = 10; // Para fins acadêmicos rigorosos, recomenda-se 30

function runExperiments() {
  if (!fs.existsSync(EXECUTABLE_PATH)) {
    console.error(`Erro: O executável '${EXECUTABLE_PATH}' não existe. Por favor, compile o projeto primeiro.`);
    process.exit(1);
  }

  if (!fs.existsSync(DATA_DIR)) {
    console.error(`Erro: O diretório de instâncias '${DATA_DIR}' não foi encontrado.`);
    process.exit(1);
  }

  // Lista e ordena os arquivos de instâncias de grafos (.txt)
  const instances = fs.readdirSync(DATA_DIR)
    .filter(file => file.endsWith('.txt'))
    .sort();

  console.log(`Encontradas ${instances.length} instâncias. Iniciando experimentos (rodadas por instância: ${RUNS_PER_INSTANCE})...\n`);

  // Cabeçalho do CSV
  const csvLines = [['Instancia', 'Vertices', 'Arestas', 'Densidade', 'Melhor_Custo', 'Media_Custo', 'Tempo_Medio_s', 'Status_Validacao']];

  for (const inst of instances) {
    const instPath = path.join(DATA_DIR, inst);
    console.log(`Testando instância: ${inst}...`);

    const costs = [];
    const times = [];
    let validationOk = true;
    let numVertices = 0;
    let numEdges = 0;
    let density = 0.0;

    for (let r = 0; r < RUNS_PER_INSTANCE; r++) {
      try {
        // Executa o executável e captura a saída
        const stdout = execSync(`"${EXECUTABLE_PATH}" "${instPath}"`, { encoding: 'utf8', stdio: ['pipe', 'pipe', 'ignore'] });

        // Parsear saídas básicas (Vértices, Arestas, Densidade) na primeira rodada
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

        // Parsear Melhor Custo Encontrado
        const costMatch = stdout.match(/Melhor custo:\s*([\d.]+)/);
        if (costMatch) {
          costs.push(parseFloat(costMatch[1]));
        }

        // Parsear Tempo Decorrido
        const timeMatch = stdout.match(/Tempo:\s*([\d.]+)s/);
        if (timeMatch) {
          times.push(parseFloat(timeMatch[1]));
        }

        // Verificar validação independente
        if (!stdout.includes('Status: OK')) {
          validationOk = false;
        }

      } catch (err) {
        console.error(`Erro ao executar a instância ${inst} na rodada ${r + 1}:`, err.message);
        validationOk = false;
      }
    }

    if (costs.length > 0) {
      const bestCost = Math.min(...costs);
      const avgCost = costs.reduce((a, b) => a + b, 0) / costs.length;
      const avgTime = times.reduce((a, b) => a + b, 0) / times.length;
      const valStatus = validationOk ? 'OK' : 'ERRO';

      csvLines.push([
        inst,
        numVertices,
        numEdges,
        density.toFixed(6),
        bestCost.toFixed(2),
        avgCost.toFixed(2),
        avgTime.toFixed(4),
        valStatus
      ]);

      console.log(` -> Concluído: ${inst} | Melhor: ${bestCost} | Média: ${avgCost.toFixed(2)} | Tempo Médio: ${avgTime.toFixed(4)}s | Validação: ${valStatus}`);
    }
  }

  // Grava o arquivo CSV
  const csvContent = csvLines.map(line => line.join(',')).join('\n');
  fs.writeFileSync(OUTPUT_CSV, csvContent, 'utf8');

  console.log(`\nExperimentos concluídos com sucesso! Resultados salvos em: ${OUTPUT_CSV}`);
}

runExperiments();
```

### Passo B: Comparação Estatística (Teste de Wilcoxon)
Assim que tiver o arquivo CSV do sistema multiagentes e um arquivo análogo gerado pelo BRKGA puro (ou outros métodos), use uma ferramenta estatística (como R, Python com Scipy ou SPSS) para rodar o **Teste de Postos Sinalizados de Wilcoxon** (Wilcoxon Signed-Rank Test) comparando o melhor custo ou custo médio em cada instância.
- Se o p-value obtido for menor que $0.05$ (nível de significância padrão), você poderá afirmar formalmente que "a abordagem multiagente é estatisticamente superior à abordagem X".

---

## 3. Sugestão de Estrutura para a Seção de Metodologia e Experimentos no Artigo

Se você estiver escrevendo um artigo ou relatório acadêmico, divida a seção correspondente em:

1. **Modelagem Matemática e Decodificação**:
   Explicar que o vetor de chaves aleatórias gerado pelos agentes possui tamanho igual ao número máximo de vértices $V$ e que cada chave é mapeada nas variáveis do problema de dominação $f(v) \in \{0, 1, 2\}$, seguido da aplicação de heurísticas de reparação locais para validar as restrições.

2. **Arquitetura Multiagente Cooperativa**:
   Descrever como os agentes (ACO, Tabu, RVNS, HHO) atuam sobre o mesmo Blackboard, onde o ACO executa a busca global, o Tabu e RVNS intensificam soluções promissoras, e o HHO acelera a convergência compartilhando a informação de melhor vetor do pool.

3. **Ambiente Computacional**:
   Mencionar a máquina de execução, sistemas de compiladores (GCC/MinGW, padrão C11/C++14 com Boost no Windows UCRT64) e paralelismo via OpenMP.

4. **Tabela de Resultados**:
   Apresentar uma tabela com as colunas: Instância, $|V|$, $|E|$, Melhor Custo SMA, Média SMA, Melhor Custo BRKGA, Tempo SMA.
