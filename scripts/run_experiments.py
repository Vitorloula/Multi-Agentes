#!/usr/bin/env python3
"""Run TRD blackboard experiments for every graph in data/.

Outputs:
  raw_runs.csv                 one row per independent execution
  agent_metrics.csv            one row per agent per execution
  convergence_events.csv       accepted blackboard publications over time
  summary.csv                  aggregated instance-level statistics
  comparison_table.csv/.tex    PLI/AG/BB comparison when references exist
  stability_table.csv/.tex     stochastic stability statistics
  cooperation_table.csv/.tex   blackboard cooperation indicators
  convergence_summary.csv      sampled mean convergence curves
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import re
import statistics
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_AGENTS = ("ACO", "Tabu Search", "RVNS", "HHO")
AGENT_COLUMNS = {
    "ACO": "aco",
    "Tabu Search": "ts",
    "RVNS": "rvns",
    "HHO": "hho",
}


@dataclass
class GraphStats:
    vertices: int
    edges: int
    density: float


def mean(values: list[float]) -> float:
    return statistics.fmean(values) if values else math.nan


def stdev(values: list[float]) -> float:
    return statistics.stdev(values) if len(values) > 1 else 0.0


def fmt(value: object) -> str:
    if value is None:
        return "--"
    if isinstance(value, float):
        if math.isnan(value):
            return "--"
        return f"{value:.4f}"
    return str(value)


def safe_name(path: Path) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", str(path))


def discover_graphs(data_dir: Path) -> list[Path]:
    ignored = {"README", "README.md", ".gitkeep"}
    graphs = []
    for path in sorted(data_dir.rglob("*")):
        if path.is_file() and path.name not in ignored and not path.name.startswith("."):
            graphs.append(path)
    return graphs


def graph_stats(path: Path) -> GraphStats:
    vertices: set[int] = set()
    edges: set[tuple[int, int]] = set()
    with path.open("r", encoding="utf-8", errors="ignore") as handle:
        for line in handle:
            parts = line.split()
            if len(parts) < 2:
                continue
            try:
                u, v = int(parts[0]), int(parts[1])
            except ValueError:
                continue
            vertices.add(u)
            vertices.add(v)
            if u != v:
                a, b = sorted((u, v))
                edges.add((a, b))

    n = len(vertices)
    m = len(edges)
    density = 0.0 if n <= 1 else (2.0 * m) / (n * (n - 1))
    return GraphStats(vertices=n, edges=m, density=density)


def load_references(path: Path | None) -> dict[str, dict[str, str]]:
    if path is None:
        return {}
    refs: dict[str, dict[str, str]] = {}
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            instance = row.get("instance") or row.get("instancia") or row.get("name")
            if not instance:
                continue
            refs[instance] = row
    return refs


def reference_for(
    refs: dict[str, dict[str, str]], instance_rel: str, instance_path: Path
) -> dict[str, str]:
    candidates = (
        instance_rel,
        instance_path.name,
        instance_path.stem,
        str(instance_path),
    )
    for candidate in candidates:
        if candidate in refs:
            return refs[candidate]
    return {}


def read_float(row: dict[str, str], *names: str) -> float | None:
    for name in names:
        value = row.get(name)
        if value not in (None, "", "--"):
            try:
                return float(value)
            except ValueError:
                pass
    return None


def parse_stdout(stdout: str) -> dict[str, float]:
    patterns = {
        "best": r"Melhor custo:\s*([0-9.+\-eE]+)",
        "elapsed": r"Tempo:\s*([0-9.+\-eE]+)s",
        "publications": r"Publicacoes aceitas:\s*(\d+)",
        "consultations": r"Consultas ao blackboard:\s*(\d+)",
        "shared_reads": r"Consultas com solucao compartilhada:\s*(\d+)",
        "time_to_best": r"Tempo ate melhor solucao:\s*([0-9.+\-eE]+)s",
    }
    parsed: dict[str, float] = {}
    for key, pattern in patterns.items():
        match = re.search(pattern, stdout)
        if match:
            parsed[key] = float(match.group(1))
    return parsed


def append_rows(path: Path, rows: Iterable[dict[str, object]], fieldnames: list[str]) -> None:
    exists = path.exists()
    with path.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        if not exists:
            writer.writeheader()
        for row in rows:
            writer.writerow(row)


def write_csv(path: Path, rows: list[dict[str, object]], fieldnames: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def ensure_build(binary: Path, max_vertices: int) -> None:
    build_dir = ROOT / "build"
    subprocess.run(
        [
            "cmake",
            "-S",
            str(ROOT),
            "-B",
            str(build_dir),
            f"-DMA_TRD_MAX_VERTICES={max_vertices}",
        ],
        check=True,
    )
    subprocess.run(["cmake", "--build", str(build_dir)], check=True)
    if not binary.exists():
        raise FileNotFoundError(f"binary not found after build: {binary}")


def run_once(
    binary: Path,
    graph: Path,
    instance_rel: str,
    run_id: int,
    output_dir: Path,
    timeout_seconds: int,
) -> tuple[dict[str, object], list[dict[str, object]], list[dict[str, object]]]:
    run_prefix = output_dir / "run_logs" / f"{safe_name(Path(instance_rel))}__run_{run_id:03d}"
    run_prefix.parent.mkdir(parents=True, exist_ok=True)
    stdout_path = run_prefix.with_suffix(".stdout.txt")
    stderr_path = run_prefix.with_suffix(".stderr.txt")
    agent_path = run_prefix.with_suffix(".agents.csv")
    convergence_path = run_prefix.with_suffix(".convergence.csv")

    agent_path.write_text(
        "agent,publications,consultations,shared_reads,best_fitness,best_time_seconds\n",
        encoding="utf-8",
    )
    convergence_path.write_text(
        "elapsed_seconds,agent,fitness,global_best,pool_size,is_new_global_best\n",
        encoding="utf-8",
    )

    env = os.environ.copy()
    env["MA_AGENT_METRICS_FILE"] = str(agent_path)
    env["MA_CONVERGENCE_FILE"] = str(convergence_path)
    env["MA_MAX_SECONDS"] = str(timeout_seconds)

    status = "ok"
    try:
        proc = subprocess.run(
            [str(binary), str(graph)],
            cwd=ROOT,
            env=env,
            capture_output=True,
            text=True,
            timeout=timeout_seconds + 30,
        )
    except subprocess.TimeoutExpired as exc:
        status = "timeout"
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""
        returncode = -1
    else:
        stdout = proc.stdout
        stderr = proc.stderr
        returncode = proc.returncode
        if returncode != 0:
            status = "error"

    stdout_path.write_text(stdout, encoding="utf-8", errors="ignore")
    stderr_path.write_text(stderr, encoding="utf-8", errors="ignore")
    parsed = parse_stdout(stdout)

    run_row: dict[str, object] = {
        "instance": instance_rel,
        "run": run_id,
        "status": status,
        "returncode": returncode,
        "best": parsed.get("best", math.nan),
        "elapsed_seconds": parsed.get("elapsed", math.nan),
        "time_to_best_seconds": parsed.get("time_to_best", math.nan),
        "publications": int(parsed.get("publications", 0)),
        "consultations": int(parsed.get("consultations", 0)),
        "shared_reads": int(parsed.get("shared_reads", 0)),
        "stdout_file": stdout_path.relative_to(output_dir),
        "stderr_file": stderr_path.relative_to(output_dir),
    }

    agent_rows: list[dict[str, object]] = []
    with agent_path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            row.update({"instance": instance_rel, "run": run_id})
            agent_rows.append(row)

    convergence_rows: list[dict[str, object]] = []
    with convergence_path.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            row.update({"instance": instance_rel, "run": run_id})
            convergence_rows.append(row)

    return run_row, agent_rows, convergence_rows


def aggregate_convergence(events: list[dict[str, object]], points: int = 101) -> list[dict[str, object]]:
    by_instance_run: dict[tuple[str, int], list[tuple[float, float]]] = {}
    max_time_by_instance: dict[str, float] = {}

    for row in events:
        instance = str(row["instance"])
        run = int(row["run"])
        elapsed = float(row["elapsed_seconds"])
        best = float(row["global_best"])
        by_instance_run.setdefault((instance, run), []).append((elapsed, best))
        max_time_by_instance[instance] = max(max_time_by_instance.get(instance, 0.0), elapsed)

    output: list[dict[str, object]] = []
    for instance, max_time in sorted(max_time_by_instance.items()):
        if max_time <= 0.0:
            continue
        times = [max_time * i / (points - 1) for i in range(points)]
        runs = {
            run: sorted(values)
            for (inst, run), values in by_instance_run.items()
            if inst == instance
        }
        for t in times:
            values = []
            for values_by_time in runs.values():
                current = math.nan
                for elapsed, best in values_by_time:
                    if elapsed <= t:
                        current = best
                    else:
                        break
                if not math.isnan(current):
                    values.append(current)
            output.append(
                {
                    "instance": instance,
                    "elapsed_seconds": t,
                    "mean_global_best": mean(values),
                    "min_global_best": min(values) if values else math.nan,
                    "max_global_best": max(values) if values else math.nan,
                    "runs_with_value": len(values),
                }
            )
    return output


def aggregate(
    raw_rows: list[dict[str, object]],
    agent_rows: list[dict[str, object]],
    stats_by_instance: dict[str, GraphStats],
    refs: dict[str, dict[str, str]],
) -> tuple[list[dict[str, object]], list[dict[str, object]], list[dict[str, object]]]:
    by_instance: dict[str, list[dict[str, object]]] = {}
    for row in raw_rows:
        if row["status"] == "ok" and not math.isnan(float(row["best"])):
            by_instance.setdefault(str(row["instance"]), []).append(row)

    agents_by_instance: dict[str, list[dict[str, object]]] = {}
    for row in agent_rows:
        agents_by_instance.setdefault(str(row["instance"]), []).append(row)

    summary_rows: list[dict[str, object]] = []
    stability_rows: list[dict[str, object]] = []
    cooperation_rows: list[dict[str, object]] = []

    for instance, rows in sorted(by_instance.items()):
        best_values = [float(row["best"]) for row in rows]
        elapsed = [float(row["elapsed_seconds"]) for row in rows]
        time_to_best = [float(row["time_to_best_seconds"]) for row in rows]
        publications = [float(row["publications"]) for row in rows]
        consultations = [float(row["consultations"]) for row in rows]
        shared_reads = [float(row["shared_reads"]) for row in rows]
        stats = stats_by_instance[instance]

        ref = reference_for(refs, instance, Path(instance))
        z_pli = read_float(ref, "z_pli", "pli", "valor_pli")
        t_pli = read_float(ref, "t_pli", "tempo_pli")
        z_ag = read_float(ref, "z_ag", "ag", "valor_ag")
        t_ag = read_float(ref, "t_ag", "tempo_ag")
        z_best = read_float(ref, "z_best", "best", "melhor_conhecido")
        denominator = z_pli if z_pli is not None else z_best
        gap = (
            100.0 * ((min(best_values) - denominator) / denominator)
            if denominator not in (None, 0.0)
            else math.nan
        )

        agent_subset = agents_by_instance.get(instance, [])
        agent_pub_means: dict[str, float] = {}
        agent_cons_means: dict[str, float] = {}
        agent_best_means: dict[str, float] = {}
        for agent in DEFAULT_AGENTS:
          agent_runs = [row for row in agent_subset if row["agent"] == agent]
          agent_pub_means[agent] = mean([float(row["publications"]) for row in agent_runs])
          agent_cons_means[agent] = mean([float(row["consultations"]) for row in agent_runs])
          agent_best_means[agent] = mean([float(row["best_fitness"]) for row in agent_runs])

        row = {
            "instance": instance,
            "vertices": stats.vertices,
            "edges": stats.edges,
            "density": stats.density,
            "runs_ok": len(rows),
            "best": min(best_values),
            "mean": mean(best_values),
            "worst": max(best_values),
            "stdev": stdev(best_values),
            "time_min": min(elapsed),
            "time_mean": mean(elapsed),
            "time_max": max(elapsed),
            "time_stdev": stdev(elapsed),
            "time_to_best_mean": mean(time_to_best),
            "publications_mean": mean(publications),
            "consultations_mean": mean(consultations),
            "shared_reads_mean": mean(shared_reads),
            "z_pli": z_pli,
            "t_pli": t_pli,
            "z_ag": z_ag,
            "t_ag": t_ag,
            "z_best_ref": z_best,
            "gap_percent": gap,
        }
        for agent, prefix in AGENT_COLUMNS.items():
            row[f"pub_{prefix}_mean"] = agent_pub_means[agent]
            row[f"cons_{prefix}_mean"] = agent_cons_means[agent]
            row[f"best_{prefix}_mean"] = agent_best_means[agent]
        summary_rows.append(row)

        stability_rows.append(
            {
                "instance": instance,
                "best": min(best_values),
                "mean": mean(best_values),
                "worst": max(best_values),
                "stdev": stdev(best_values),
                "time_mean": mean(elapsed),
            }
        )
        cooperation_rows.append(
            {
                "instance": instance,
                "publications": mean(publications),
                "consultations": mean(consultations),
                "pub_aco": agent_pub_means["ACO"],
                "pub_ts": agent_pub_means["Tabu Search"],
                "pub_rvns": agent_pub_means["RVNS"],
                "pub_hho": agent_pub_means["HHO"],
            }
        )

    return summary_rows, stability_rows, cooperation_rows


def write_latex_table(path: Path, caption: str, label: str, headers: list[str], rows: list[list[object]]) -> None:
    alignment = "l" + "r" * (len(headers) - 1)
    lines = [
        r"\begin{table}[h!]",
        r"\centering",
        rf"\caption{{{caption}}}",
        rf"\label{{{label}}}",
        r"\scriptsize",
        r"\setlength{\tabcolsep}{4pt}",
        r"\renewcommand{\arraystretch}{1.15}",
        rf"\begin{{tabular}}{{{alignment}}}",
        r"\hline",
        " & ".join(rf"\textbf{{{header}}}" for header in headers) + r" \\",
        r"\hline",
    ]
    for row in rows:
        lines.append(" & ".join(fmt(value) for value in row) + r" \\")
    lines.extend([r"\hline", r"\end{tabular}", r"\end{table}", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def write_tables(
    output_dir: Path,
    summary_rows: list[dict[str, object]],
    stability_rows: list[dict[str, object]],
    cooperation_rows: list[dict[str, object]],
) -> None:
    comparison_rows = [
        {
            "instance": row["instance"],
            "z_pli": row["z_pli"],
            "t_pli": row["t_pli"],
            "z_ag": row["z_ag"],
            "t_ag": row["t_ag"],
            "z_bb": row["best"],
            "t_bb": row["time_mean"],
            "gap_percent": row["gap_percent"],
        }
        for row in summary_rows
    ]
    write_csv(
        output_dir / "comparison_table.csv",
        comparison_rows,
        ["instance", "z_pli", "t_pli", "z_ag", "t_ag", "z_bb", "t_bb", "gap_percent"],
    )
    write_csv(
        output_dir / "stability_table.csv",
        stability_rows,
        ["instance", "best", "mean", "worst", "stdev", "time_mean"],
    )
    write_csv(
        output_dir / "cooperation_table.csv",
        cooperation_rows,
        ["instance", "publications", "consultations", "pub_aco", "pub_ts", "pub_rvns", "pub_hho"],
    )

    write_latex_table(
        output_dir / "comparison_table.tex",
        "Comparacao dos resultados obtidos para as instancias do PDRT.",
        "tab:comparacao-resultados",
        [
            "Instancia",
            "$z_{\\mathrm{PLI}}$",
            "$t_{\\mathrm{PLI}}$",
            "$z_{\\mathrm{AG}}$",
            "$t_{\\mathrm{AG}}$",
            "$z_{\\mathrm{BB}}$",
            "$t_{\\mathrm{BB}}$",
            "gap(\\%)",
        ],
        [
            [
                row["instance"],
                row["z_pli"],
                row["t_pli"],
                row["z_ag"],
                row["t_ag"],
                row["z_bb"],
                row["t_bb"],
                row["gap_percent"],
            ]
            for row in comparison_rows
        ],
    )
    write_latex_table(
        output_dir / "stability_table.tex",
        "Estatisticas das execucoes independentes da arquitetura proposta.",
        "tab:estatisticas-execucoes",
        ["Instancia", "Melhor", "Media", "Pior", "Desvio", "Tempo medio"],
        [
            [
                row["instance"],
                row["best"],
                row["mean"],
                row["worst"],
                row["stdev"],
                row["time_mean"],
            ]
            for row in stability_rows
        ],
    )
    write_latex_table(
        output_dir / "cooperation_table.tex",
        "Indicadores de cooperacao observados durante a execucao.",
        "tab:indicadores-cooperacao",
        ["Instancia", "Pub.", "Cons.", "Pub. ACO", "Pub. TS", "Pub. RVNS", "Pub. HHO"],
        [
            [
                row["instance"],
                row["publications"],
                row["consultations"],
                row["pub_aco"],
                row["pub_ts"],
                row["pub_rvns"],
                row["pub_hho"],
            ]
            for row in cooperation_rows
        ],
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default="data", type=Path)
    parser.add_argument("--binary", default="build/multi_agentes", type=Path)
    parser.add_argument("--runs", default=30, type=int)
    parser.add_argument("--timeout-seconds", default=900, type=int)
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--reference-csv", type=Path)
    parser.add_argument("--no-build", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    data_dir = (ROOT / args.data_dir).resolve() if not args.data_dir.is_absolute() else args.data_dir
    binary = (ROOT / args.binary).resolve() if not args.binary.is_absolute() else args.binary
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_dir = args.output_dir or (ROOT / "results" / f"trd_{timestamp}")
    output_dir.mkdir(parents=True, exist_ok=True)

    graphs = discover_graphs(data_dir)
    if not graphs:
        print(f"no graph files found in {data_dir}", file=sys.stderr)
        return 1

    stats_by_instance: dict[str, GraphStats] = {}
    max_vertices = 0
    for graph in graphs:
        instance_rel = graph.relative_to(data_dir).as_posix()
        stats = graph_stats(graph)
        stats_by_instance[instance_rel] = stats
        max_vertices = max(max_vertices, stats.vertices)

    if not args.no_build:
        ensure_build(binary, max_vertices)

    refs = load_references(args.reference_csv)
    raw_rows: list[dict[str, object]] = []
    agent_rows: list[dict[str, object]] = []
    convergence_rows: list[dict[str, object]] = []

    raw_fields = [
        "instance",
        "run",
        "status",
        "returncode",
        "vertices",
        "edges",
        "density",
        "best",
        "elapsed_seconds",
        "time_to_best_seconds",
        "publications",
        "consultations",
        "shared_reads",
        "stdout_file",
        "stderr_file",
    ]
    agent_fields = [
        "instance",
        "run",
        "agent",
        "publications",
        "consultations",
        "shared_reads",
        "best_fitness",
        "best_time_seconds",
    ]
    convergence_fields = [
        "instance",
        "run",
        "elapsed_seconds",
        "agent",
        "fitness",
        "global_best",
        "pool_size",
        "is_new_global_best",
    ]

    for graph in graphs:
        instance_rel = graph.relative_to(data_dir).as_posix()
        stats = stats_by_instance[instance_rel]
        print(f"[{instance_rel}] vertices={stats.vertices} edges={stats.edges}")

        for run_id in range(1, args.runs + 1):
            print(f"  run {run_id}/{args.runs}")
            run_row, run_agent_rows, run_convergence_rows = run_once(
                binary, graph, instance_rel, run_id, output_dir, args.timeout_seconds
            )
            run_row.update(
                {
                    "vertices": stats.vertices,
                    "edges": stats.edges,
                    "density": stats.density,
                }
            )
            raw_rows.append(run_row)
            agent_rows.extend(run_agent_rows)
            convergence_rows.extend(run_convergence_rows)
            append_rows(output_dir / "raw_runs.csv", [run_row], raw_fields)
            append_rows(output_dir / "agent_metrics.csv", run_agent_rows, agent_fields)
            append_rows(
                output_dir / "convergence_events.csv",
                run_convergence_rows,
                convergence_fields,
            )

    summary_rows, stability_rows, cooperation_rows = aggregate(
        raw_rows, agent_rows, stats_by_instance, refs
    )
    summary_fields = list(summary_rows[0].keys()) if summary_rows else []
    if summary_fields:
        write_csv(output_dir / "summary.csv", summary_rows, summary_fields)
    write_tables(output_dir, summary_rows, stability_rows, cooperation_rows)

    convergence_summary = aggregate_convergence(convergence_rows)
    if convergence_summary:
        write_csv(
            output_dir / "convergence_summary.csv",
            convergence_summary,
            [
                "instance",
                "elapsed_seconds",
                "mean_global_best",
                "min_global_best",
                "max_global_best",
                "runs_with_value",
            ],
        )

    print(f"\nResultados salvos em: {output_dir}")
    print("Arquivos principais: summary.csv, raw_runs.csv, agent_metrics.csv, convergence_events.csv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
