#!/usr/bin/env python3
"""
Real-World Audio Testing Results Analyzer
Generates additional insights and visualizations from test results
"""

import re
import json
from collections import defaultdict
from typing import Dict, List, Tuple

class RealWorldResultsAnalyzer:
    """Analyzes real-world audio testing results"""

    def __init__(self, report_file: str = "REAL_WORLD_AUDIO_TESTING_REPORT.md"):
        self.report_file = report_file
        self.results = []
        self.engine_names = {}

    def parse_report(self):
        """Parse the markdown report"""
        print(f"Parsing {self.report_file}...")

        with open(self.report_file, 'r') as f:
            content = f.read()

        # Extract engine results
        engine_pattern = r'### Engine (\d+): (.+?)\n\n\*\*Overall Grade\*\*: ([A-F])'
        engines = re.findall(engine_pattern, content)

        for engine_id, engine_name, overall_grade in engines:
            self.engine_names[int(engine_id)] = engine_name

            # Find material results for this engine
            material_section = re.search(
                rf'### Engine {engine_id}:.*?\n\| Material \| Grade \| Issues \|\n\|.*?\n((?:\|.*?\n)+)',
                content,
                re.DOTALL
            )

            if material_section:
                material_lines = material_section.group(1).strip().split('\n')
                for line in material_lines:
                    parts = [p.strip() for p in line.split('|')[1:-1]]
                    if len(parts) == 3:
                        material, grade, issues = parts
                        self.results.append({
                            'engine_id': int(engine_id),
                            'engine_name': engine_name,
                            'material': material,
                            'grade': grade,
                            'issues': issues,
                            'overall_grade': overall_grade
                        })

        print(f"Parsed {len(self.results)} test results from {len(self.engine_names)} engines")

    def analyze_by_category(self):
        """Analyze results by engine category"""
        categories = {
            'Dynamics & Compression': list(range(1, 7)),
            'Filters & EQ': list(range(7, 15)),
            'Distortion & Saturation': list(range(15, 24)),
            'Modulation': list(range(24, 34)),
            'Reverb & Delay': list(range(34, 44)),
            'Spatial & Special': list(range(44, 53)),
            'Utility': list(range(53, 57))
        }

        print("\n" + "=" * 70)
        print("RESULTS BY CATEGORY")
        print("=" * 70)

        for category, engine_ids in categories.items():
            grades = defaultdict(int)
            total = 0

            for result in self.results:
                if result['engine_id'] in engine_ids:
                    grades[result['grade']] += 1
                    total += 1

            if total == 0:
                continue

            print(f"\n{category}")
            print("-" * 70)

            for grade in ['A', 'B', 'C', 'D', 'F']:
                count = grades[grade]
                percent = (count * 100.0 / total) if total > 0 else 0
                bar = '█' * int(percent / 2)
                print(f"  {grade}: {count:3d} ({percent:5.1f}%) {bar}")

            # Calculate average score
            score_map = {'A': 95, 'B': 85, 'C': 75, 'D': 65, 'F': 45}
            avg_score = sum(score_map[g] * c for g, c in grades.items()) / total
            print(f"\n  Average Score: {avg_score:.1f}/100")

    def analyze_by_material(self):
        """Analyze results by test material"""
        print("\n" + "=" * 70)
        print("RESULTS BY TEST MATERIAL")
        print("=" * 70)

        materials = {}
        for result in self.results:
            mat = result['material']
            if mat not in materials:
                materials[mat] = defaultdict(int)
            materials[mat][result['grade']] += 1

        for material in sorted(materials.keys()):
            grades = materials[material]
            total = sum(grades.values())

            print(f"\n{material}")
            print("-" * 70)

            for grade in ['A', 'B', 'C', 'D', 'F']:
                count = grades[grade]
                percent = (count * 100.0 / total) if total > 0 else 0
                bar = '█' * int(percent / 2)
                print(f"  {grade}: {count:2d} ({percent:5.1f}%) {bar}")

            # Most challenging for which engines?
            if grades['F'] > 0:
                problem_engines = [r['engine_name'] for r in self.results
                                 if r['material'] == material and r['grade'] == 'F']
                print(f"\n  ⚠️  Problem engines: {', '.join(problem_engines)}")

    def find_problem_patterns(self):
        """Identify common problem patterns"""
        print("\n" + "=" * 70)
        print("COMMON PROBLEM PATTERNS")
        print("=" * 70)

        issue_patterns = defaultdict(list)

        for result in self.results:
            if result['grade'] in ['D', 'F'] and result['issues'] != 'None':
                issues = result['issues']

                # Extract issue types
                if 'clipping' in issues.lower():
                    issue_patterns['Clipping'].append(result)
                if 'artifacts' in issues.lower() or 'discontinuities' in issues.lower():
                    issue_patterns['Artifacts'].append(result)
                if 'silent' in issues.lower() or 'silence' in issues.lower():
                    issue_patterns['Silence'].append(result)
                if 'dc offset' in issues.lower():
                    issue_patterns['DC Offset'].append(result)
                if 'dynamic range' in issues.lower():
                    issue_patterns['Dynamic Range Loss'].append(result)
                if 'noise' in issues.lower():
                    issue_patterns['Noise'].append(result)

        for issue_type, occurrences in sorted(issue_patterns.items(),
                                              key=lambda x: len(x[1]),
                                              reverse=True):
            print(f"\n{issue_type}: {len(occurrences)} occurrences")
            print("-" * 70)

            # List affected engines (unique)
            affected = {}
            for result in occurrences:
                engine = f"Engine {result['engine_id']}: {result['engine_name']}"
                if engine not in affected:
                    affected[engine] = []
                affected[engine].append(result['material'])

            for engine, materials in sorted(affected.items())[:10]:  # Top 10
                mat_list = ', '.join(materials[:3])
                if len(materials) > 3:
                    mat_list += f" (+{len(materials)-3} more)"
                print(f"  • {engine}")
                print(f"    Materials: {mat_list}")

    def identify_champions(self):
        """Find best-performing engines"""
        print("\n" + "=" * 70)
        print("CHAMPION ENGINES (Perfect Scores)")
        print("=" * 70)

        engine_grades = defaultdict(list)
        for result in self.results:
            engine_grades[result['engine_id']].append(result['grade'])

        champions = []
        for engine_id, grades in engine_grades.items():
            if all(g == 'A' for g in grades):
                champions.append(engine_id)

        if champions:
            print("\nEngines with Grade A on ALL materials:")
            for engine_id in sorted(champions):
                print(f"  ⭐ Engine {engine_id}: {self.engine_names[engine_id]}")
        else:
            print("\nNo engines achieved perfect scores on all materials.")

        # High performers (mostly A's)
        print("\nHigh Performers (≥90% Grade A):")
        for engine_id, grades in sorted(engine_grades.items()):
            a_count = grades.count('A')
            a_percent = (a_count * 100.0) / len(grades)
            if a_percent >= 90 and a_percent < 100:
                print(f"  • Engine {engine_id}: {self.engine_names[engine_id]} "
                      f"({a_percent:.0f}% A grades)")

    def identify_problem_engines(self):
        """Find engines needing attention"""
        print("\n" + "=" * 70)
        print("ENGINES REQUIRING ATTENTION")
        print("=" * 70)

        engine_grades = defaultdict(list)
        for result in self.results:
            engine_grades[result['engine_id']].append(result['grade'])

        critical = []
        warning = []

        for engine_id, grades in engine_grades.items():
            f_count = grades.count('F')
            d_count = grades.count('D')

            if f_count > 0:
                critical.append((engine_id, f_count, len(grades)))
            elif d_count >= len(grades) / 2:  # Mostly D's
                warning.append((engine_id, d_count, len(grades)))

        if critical:
            print("\n🚨 CRITICAL (Grade F failures):")
            for engine_id, f_count, total in sorted(critical, key=lambda x: x[1], reverse=True):
                print(f"  • Engine {engine_id}: {self.engine_names[engine_id]} "
                      f"({f_count}/{total} failures)")

                # Show which materials failed
                failed_materials = [r['material'] for r in self.results
                                  if r['engine_id'] == engine_id and r['grade'] == 'F']
                print(f"    Failed: {', '.join(failed_materials)}")
        else:
            print("\n✅ No critical failures detected!")

        if warning:
            print("\n⚠️  WARNING (Mostly Grade D):")
            for engine_id, d_count, total in sorted(warning, key=lambda x: x[1], reverse=True):
                print(f"  • Engine {engine_id}: {self.engine_names[engine_id]} "
                      f"({d_count}/{total} D grades)")

    def generate_json_export(self, output_file: str = "real_world_test_results.json"):
        """Export results as JSON for further analysis"""
        data = {
            'test_info': {
                'total_engines': len(self.engine_names),
                'total_materials': len(set(r['material'] for r in self.results)),
                'total_tests': len(self.results)
            },
            'engines': {},
            'materials': {},
            'results': self.results
        }

        # Aggregate by engine
        for engine_id, engine_name in self.engine_names.items():
            engine_results = [r for r in self.results if r['engine_id'] == engine_id]
            grades = [r['grade'] for r in engine_results]

            data['engines'][engine_id] = {
                'name': engine_name,
                'overall_grade': engine_results[0]['overall_grade'] if engine_results else 'N/A',
                'grade_counts': {
                    'A': grades.count('A'),
                    'B': grades.count('B'),
                    'C': grades.count('C'),
                    'D': grades.count('D'),
                    'F': grades.count('F')
                }
            }

        # Aggregate by material
        for material in set(r['material'] for r in self.results):
            material_results = [r for r in self.results if r['material'] == material]
            grades = [r['grade'] for r in material_results]

            data['materials'][material] = {
                'grade_counts': {
                    'A': grades.count('A'),
                    'B': grades.count('B'),
                    'C': grades.count('C'),
                    'D': grades.count('D'),
                    'F': grades.count('F')
                }
            }

        with open(output_file, 'w') as f:
            json.dump(data, f, indent=2)

        print(f"\n📊 Exported results to {output_file}")

    def generate_summary_report(self):
        """Generate a concise summary report"""
        print("\n" + "=" * 70)
        print("EXECUTIVE SUMMARY")
        print("=" * 70)

        total_tests = len(self.results)
        grade_counts = defaultdict(int)
        for result in self.results:
            grade_counts[result['grade']] += 1

        print(f"\nTotal Tests: {total_tests}")
        print(f"Engines Tested: {len(self.engine_names)}")
        print(f"Materials Used: {len(set(r['material'] for r in self.results))}")
        print("\nGrade Distribution:")

        for grade in ['A', 'B', 'C', 'D', 'F']:
            count = grade_counts[grade]
            percent = (count * 100.0 / total_tests)
            print(f"  {grade}: {count:3d} ({percent:5.1f}%)")

        pass_count = total_tests - grade_counts['F']
        pass_rate = (pass_count * 100.0 / total_tests)
        print(f"\nPass Rate: {pass_rate:.1f}% ({pass_count}/{total_tests})")

        # Quality assessment
        if pass_rate >= 95:
            print("\n✅ EXCELLENT - Production ready")
        elif pass_rate >= 90:
            print("\n✅ GOOD - Minor issues to address")
        elif pass_rate >= 80:
            print("\n⚠️  ACCEPTABLE - Several engines need attention")
        else:
            print("\n🚨 CRITICAL - Significant quality issues detected")

def main():
    """Main analysis workflow"""
    print("=" * 70)
    print("REAL-WORLD AUDIO TESTING - RESULTS ANALYSIS")
    print("=" * 70)

    analyzer = RealWorldResultsAnalyzer()

    try:
        analyzer.parse_report()
    except FileNotFoundError:
        print("\nERROR: REAL_WORLD_AUDIO_TESTING_REPORT.md not found!")
        print("Run ./test_real_world_audio first to generate results.")
        return

    analyzer.generate_summary_report()
    analyzer.analyze_by_category()
    analyzer.analyze_by_material()
    analyzer.find_problem_patterns()
    analyzer.identify_champions()
    analyzer.identify_problem_engines()
    analyzer.generate_json_export()

    print("\n" + "=" * 70)
    print("ANALYSIS COMPLETE")
    print("=" * 70)

if __name__ == "__main__":
    main()
