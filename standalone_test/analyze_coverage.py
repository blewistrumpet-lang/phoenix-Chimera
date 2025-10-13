#!/usr/bin/env python3
"""
ChimeraPhoenix Coverage Analysis Tool
Analyzes LLVM coverage data to identify untested code paths
Generates comprehensive reports on line, branch, and function coverage
"""

import json
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Tuple

class CoverageAnalyzer:
    def __init__(self, json_file: str):
        self.json_file = json_file
        self.data = None
        self.file_coverage = {}
        self.total_stats = {
            'lines_covered': 0,
            'lines_total': 0,
            'branches_covered': 0,
            'branches_total': 0,
            'functions_covered': 0,
            'functions_total': 0
        }

    def load_data(self):
        """Load coverage JSON data"""
        try:
            with open(self.json_file, 'r') as f:
                self.data = json.load(f)
            return True
        except FileNotFoundError:
            print(f"ERROR: Coverage JSON file not found: {self.json_file}")
            return False
        except json.JSONDecodeError as e:
            print(f"ERROR: Invalid JSON in coverage file: {e}")
            return False

    def analyze(self):
        """Analyze coverage data"""
        if not self.data:
            return False

        # Process each file in the coverage data
        for file_data in self.data.get('data', []):
            for file_info in file_data.get('files', []):
                filename = Path(file_info['filename']).name

                # Initialize file coverage stats
                self.file_coverage[filename] = {
                    'path': file_info['filename'],
                    'lines_covered': 0,
                    'lines_total': 0,
                    'branches_covered': 0,
                    'branches_total': 0,
                    'functions_covered': 0,
                    'functions_total': 0,
                    'uncovered_lines': [],
                    'uncovered_functions': [],
                    'uncovered_branches': []
                }

                stats = file_info.get('summary', {})

                # Line coverage
                line_stats = stats.get('lines', {})
                lines_covered = line_stats.get('covered', 0)
                lines_total = line_stats.get('count', 0)
                self.file_coverage[filename]['lines_covered'] = lines_covered
                self.file_coverage[filename]['lines_total'] = lines_total
                self.total_stats['lines_covered'] += lines_covered
                self.total_stats['lines_total'] += lines_total

                # Branch coverage
                branch_stats = stats.get('branches', {})
                branches_covered = branch_stats.get('covered', 0)
                branches_total = branch_stats.get('count', 0)
                self.file_coverage[filename]['branches_covered'] = branches_covered
                self.file_coverage[filename]['branches_total'] = branches_total
                self.total_stats['branches_covered'] += branches_covered
                self.total_stats['branches_total'] += branches_total

                # Function coverage
                function_stats = stats.get('functions', {})
                functions_covered = function_stats.get('covered', 0)
                functions_total = function_stats.get('count', 0)
                self.file_coverage[filename]['functions_covered'] = functions_covered
                self.file_coverage[filename]['functions_total'] = functions_total
                self.total_stats['functions_covered'] += functions_covered
                self.total_stats['functions_total'] += functions_total

                # Identify uncovered lines
                for segment in file_info.get('segments', []):
                    line = segment[0]
                    col = segment[1]
                    count = segment[2]
                    is_region_entry = segment[3]

                    if is_region_entry and count == 0:
                        self.file_coverage[filename]['uncovered_lines'].append(line)

                # Identify uncovered functions
                for function in file_info.get('functions', []):
                    if function.get('count', 0) == 0:
                        func_name = function.get('name', 'unknown')
                        func_line = function.get('regions', [[0]])[0][0] if function.get('regions') else 0
                        self.file_coverage[filename]['uncovered_functions'].append({
                            'name': func_name,
                            'line': func_line
                        })

        return True

    def calculate_percentages(self) -> Dict[str, float]:
        """Calculate coverage percentages"""
        percentages = {}

        if self.total_stats['lines_total'] > 0:
            percentages['line_coverage'] = (self.total_stats['lines_covered'] /
                                           self.total_stats['lines_total']) * 100
        else:
            percentages['line_coverage'] = 0.0

        if self.total_stats['branches_total'] > 0:
            percentages['branch_coverage'] = (self.total_stats['branches_covered'] /
                                             self.total_stats['branches_total']) * 100
        else:
            percentages['branch_coverage'] = 0.0

        if self.total_stats['functions_total'] > 0:
            percentages['function_coverage'] = (self.total_stats['functions_covered'] /
                                               self.total_stats['functions_total']) * 100
        else:
            percentages['function_coverage'] = 0.0

        return percentages

    def generate_report(self, output_file: str):
        """Generate comprehensive coverage report"""
        with open(output_file, 'w') as f:
            f.write("=" * 80 + "\n")
            f.write("ChimeraPhoenix Code Coverage Analysis Report\n")
            f.write("=" * 80 + "\n\n")

            # Overall summary
            percentages = self.calculate_percentages()
            f.write("OVERALL COVERAGE SUMMARY\n")
            f.write("-" * 80 + "\n")
            f.write(f"Line Coverage:     {self.total_stats['lines_covered']:6d} / {self.total_stats['lines_total']:6d} "
                   f"({percentages['line_coverage']:6.2f}%)\n")
            f.write(f"Branch Coverage:   {self.total_stats['branches_covered']:6d} / {self.total_stats['branches_total']:6d} "
                   f"({percentages['branch_coverage']:6.2f}%)\n")
            f.write(f"Function Coverage: {self.total_stats['functions_covered']:6d} / {self.total_stats['functions_total']:6d} "
                   f"({percentages['function_coverage']:6.2f}%)\n")
            f.write("\n")

            # Per-file breakdown
            f.write("PER-FILE COVERAGE BREAKDOWN\n")
            f.write("-" * 80 + "\n\n")

            # Sort files by coverage percentage
            sorted_files = sorted(
                self.file_coverage.items(),
                key=lambda x: (x[1]['lines_covered'] / max(x[1]['lines_total'], 1)),
                reverse=True
            )

            for filename, stats in sorted_files:
                if stats['lines_total'] == 0:
                    continue

                line_pct = (stats['lines_covered'] / stats['lines_total']) * 100
                branch_pct = (stats['branches_covered'] / max(stats['branches_total'], 1)) * 100
                func_pct = (stats['functions_covered'] / max(stats['functions_total'], 1)) * 100

                f.write(f"{filename}\n")
                f.write(f"  Lines:     {stats['lines_covered']:4d}/{stats['lines_total']:4d} ({line_pct:5.1f}%)\n")
                f.write(f"  Branches:  {stats['branches_covered']:4d}/{stats['branches_total']:4d} ({branch_pct:5.1f}%)\n")
                f.write(f"  Functions: {stats['functions_covered']:4d}/{stats['functions_total']:4d} ({func_pct:5.1f}%)\n")

                # Show uncovered functions
                if stats['uncovered_functions']:
                    f.write(f"  Uncovered Functions ({len(stats['uncovered_functions'])}):\n")
                    for func in stats['uncovered_functions'][:10]:  # Limit to first 10
                        f.write(f"    - {func['name']} (line {func['line']})\n")
                    if len(stats['uncovered_functions']) > 10:
                        f.write(f"    ... and {len(stats['uncovered_functions']) - 10} more\n")

                f.write("\n")

            # Low coverage files
            f.write("\n" + "=" * 80 + "\n")
            f.write("FILES WITH LOW COVERAGE (< 50%)\n")
            f.write("-" * 80 + "\n\n")

            low_coverage_files = []
            for filename, stats in self.file_coverage.items():
                if stats['lines_total'] > 0:
                    coverage = (stats['lines_covered'] / stats['lines_total']) * 100
                    if coverage < 50:
                        low_coverage_files.append((filename, coverage, stats))

            low_coverage_files.sort(key=lambda x: x[1])

            if low_coverage_files:
                for filename, coverage, stats in low_coverage_files:
                    f.write(f"{filename}: {coverage:.1f}% "
                           f"({stats['lines_covered']}/{stats['lines_total']} lines)\n")
            else:
                f.write("No files with coverage below 50%\n")

            # Untested engines
            f.write("\n" + "=" * 80 + "\n")
            f.write("UNTESTED OR PARTIALLY TESTED ENGINES\n")
            f.write("-" * 80 + "\n\n")

            untested = []
            for filename, stats in self.file_coverage.items():
                if stats['lines_total'] > 0:
                    coverage = (stats['lines_covered'] / stats['lines_total']) * 100
                    if coverage < 70:  # Less than 70% coverage
                        untested.append((filename, coverage))

            untested.sort(key=lambda x: x[1])

            if untested:
                for filename, coverage in untested:
                    f.write(f"  • {filename}: {coverage:.1f}% coverage\n")
            else:
                f.write("All engines have >= 70% coverage\n")

            # Recommendations
            f.write("\n" + "=" * 80 + "\n")
            f.write("RECOMMENDATIONS FOR IMPROVEMENT\n")
            f.write("-" * 80 + "\n\n")

            if percentages['line_coverage'] < 80:
                f.write("• Increase line coverage to at least 80%\n")
                f.write("  - Focus on files with < 50% coverage\n")
                f.write("  - Add tests for edge cases and error handling\n\n")

            if percentages['branch_coverage'] < 70:
                f.write("• Improve branch coverage to at least 70%\n")
                f.write("  - Test all conditional branches (if/else, switch cases)\n")
                f.write("  - Test boundary conditions\n\n")

            if percentages['function_coverage'] < 90:
                f.write("• Increase function coverage to at least 90%\n")
                f.write("  - Ensure all public API functions are tested\n")
                f.write("  - Test helper functions and utilities\n\n")

            f.write("\n")

    def generate_html_dashboard(self, output_file: str):
        """Generate interactive HTML dashboard"""
        percentages = self.calculate_percentages()

        html = f"""<!DOCTYPE html>
<html>
<head>
    <title>ChimeraPhoenix Coverage Dashboard</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            margin: 20px;
            background: #f5f5f5;
        }}
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        h1 {{
            color: #333;
            border-bottom: 3px solid #4CAF50;
            padding-bottom: 10px;
        }}
        .summary {{
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 20px;
            margin: 30px 0;
        }}
        .metric {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 8px;
            text-align: center;
        }}
        .metric-value {{
            font-size: 48px;
            font-weight: bold;
            margin: 10px 0;
        }}
        .metric-label {{
            font-size: 14px;
            opacity: 0.9;
        }}
        .progress-bar {{
            background: #e0e0e0;
            height: 30px;
            border-radius: 15px;
            overflow: hidden;
            margin: 10px 0;
        }}
        .progress-fill {{
            height: 100%;
            background: linear-gradient(90deg, #4CAF50, #8BC34A);
            transition: width 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: bold;
        }}
        .file-list {{
            margin-top: 30px;
        }}
        .file-item {{
            padding: 15px;
            border: 1px solid #ddd;
            margin: 10px 0;
            border-radius: 5px;
            background: #fafafa;
        }}
        .file-name {{
            font-weight: bold;
            color: #333;
            margin-bottom: 10px;
        }}
        .file-stats {{
            display: flex;
            gap: 20px;
            font-size: 14px;
        }}
        .low-coverage {{
            border-left: 4px solid #f44336;
        }}
        .medium-coverage {{
            border-left: 4px solid #ff9800;
        }}
        .high-coverage {{
            border-left: 4px solid #4CAF50;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>ChimeraPhoenix Code Coverage Dashboard</h1>

        <div class="summary">
            <div class="metric">
                <div class="metric-label">Line Coverage</div>
                <div class="metric-value">{percentages['line_coverage']:.1f}%</div>
                <div class="metric-label">{self.total_stats['lines_covered']} / {self.total_stats['lines_total']} lines</div>
            </div>
            <div class="metric">
                <div class="metric-label">Branch Coverage</div>
                <div class="metric-value">{percentages['branch_coverage']:.1f}%</div>
                <div class="metric-label">{self.total_stats['branches_covered']} / {self.total_stats['branches_total']} branches</div>
            </div>
            <div class="metric">
                <div class="metric-label">Function Coverage</div>
                <div class="metric-value">{percentages['function_coverage']:.1f}%</div>
                <div class="metric-label">{self.total_stats['functions_covered']} / {self.total_stats['functions_total']} functions</div>
            </div>
        </div>

        <h2>Overall Progress</h2>
        <div class="progress-bar">
            <div class="progress-fill" style="width: {percentages['line_coverage']:.1f}%">
                {percentages['line_coverage']:.1f}%
            </div>
        </div>

        <h2>File Coverage Details</h2>
        <div class="file-list">
"""

        # Sort files by coverage
        sorted_files = sorted(
            self.file_coverage.items(),
            key=lambda x: (x[1]['lines_covered'] / max(x[1]['lines_total'], 1))
        )

        for filename, stats in sorted_files:
            if stats['lines_total'] == 0:
                continue

            line_pct = (stats['lines_covered'] / stats['lines_total']) * 100

            coverage_class = 'low-coverage' if line_pct < 50 else 'medium-coverage' if line_pct < 80 else 'high-coverage'

            html += f"""
            <div class="file-item {coverage_class}">
                <div class="file-name">{filename}</div>
                <div class="file-stats">
                    <span>Lines: {stats['lines_covered']}/{stats['lines_total']} ({line_pct:.1f}%)</span>
                    <span>Branches: {stats['branches_covered']}/{stats['branches_total']}</span>
                    <span>Functions: {stats['functions_covered']}/{stats['functions_total']}</span>
                </div>
                <div class="progress-bar" style="height: 20px; margin-top: 10px;">
                    <div class="progress-fill" style="width: {line_pct:.1f}%; font-size: 12px;">
                        {line_pct:.1f}%
                    </div>
                </div>
            </div>
"""

        html += """
        </div>
    </div>
</body>
</html>
"""

        with open(output_file, 'w') as f:
            f.write(html)

def main():
    print("ChimeraPhoenix Coverage Analyzer")
    print("=" * 60)
    print()

    json_file = "build_coverage/coverage/coverage_export.json"

    if len(sys.argv) > 1:
        json_file = sys.argv[1]

    analyzer = CoverageAnalyzer(json_file)

    print(f"Loading coverage data from: {json_file}")
    if not analyzer.load_data():
        sys.exit(1)

    print("Analyzing coverage...")
    if not analyzer.analyze():
        print("ERROR: Failed to analyze coverage data")
        sys.exit(1)

    print("Generating reports...")

    # Generate text report
    report_file = "build_coverage/coverage/analysis_report.txt"
    analyzer.generate_report(report_file)
    print(f"  ✓ Text report: {report_file}")

    # Generate HTML dashboard
    dashboard_file = "build_coverage/coverage/dashboard.html"
    analyzer.generate_html_dashboard(dashboard_file)
    print(f"  ✓ HTML dashboard: {dashboard_file}")

    print()
    print("=" * 60)
    print("COVERAGE SUMMARY")
    print("=" * 60)

    percentages = analyzer.calculate_percentages()
    stats = analyzer.total_stats

    print(f"Line Coverage:     {stats['lines_covered']:6d} / {stats['lines_total']:6d} ({percentages['line_coverage']:6.2f}%)")
    print(f"Branch Coverage:   {stats['branches_covered']:6d} / {stats['branches_total']:6d} ({percentages['branch_coverage']:6.2f}%)")
    print(f"Function Coverage: {stats['functions_covered']:6d} / {stats['functions_total']:6d} ({percentages['function_coverage']:6.2f}%)")
    print()

    print("View reports:")
    print(f"  • Text:      cat {report_file}")
    print(f"  • Dashboard: open {dashboard_file}")
    print()

if __name__ == "__main__":
    main()
