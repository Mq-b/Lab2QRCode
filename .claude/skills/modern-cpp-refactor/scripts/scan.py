#!/usr/bin/env python3
"""
Modern C++ Code Scanner
Detects legacy C/C++98/11 patterns that should be modernized to C++17/20
"""

import sys
import os
import re
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Tuple
from collections import defaultdict

@dataclass
class Issue:
    """Represents a detected legacy pattern"""
    file_path: str
    line_number: int
    category: str
    severity: str  # 'error', 'warning', 'info'
    legacy_pattern: str
    modern_alternative: str
    code_snippet: str

class ModernCppScanner:
    """Scans C++ code for legacy patterns"""

    def __init__(self, detailed: bool = False):
        self.detailed = detailed
        self.issues: List[Issue] = []
        self.stats: Dict[str, int] = defaultdict(int)

        # Define patterns to detect
        self.patterns = [
            # Memory Management
            (r'\bnew\s+\w+(?:\s*\[|\s*\()', 'error', 'Memory Management',
             'Raw new/delete', 'Use std::make_unique<T>() or std::make_shared<T>()'),

            (r'\bdelete\s+', 'error', 'Memory Management',
             'Manual delete', 'Use smart pointers (RAII)'),

            (r'\bdelete\s*\[\s*\]', 'error', 'Memory Management',
             'Manual delete[]', 'Use std::vector<T> or std::array<T, N>'),

            (r'\bmalloc\s*\(', 'error', 'Memory Management',
             'C-style malloc', 'Use std::vector or smart pointers'),

            (r'\bfree\s*\(', 'error', 'Memory Management',
             'C-style free', 'Use RAII containers'),

            # Pointers and Nulls
            (r'\bNULL\b', 'warning', 'Type Safety',
             'NULL macro', 'Use nullptr'),

            # Arrays
            (r'\b\w+\s+\w+\s*\[\s*\d+\s*\]', 'warning', 'Containers',
             'C-style array', 'Use std::array<T, N> or std::vector<T>'),

            # String handling
            (r'\bchar\s+\w+\s*\[\s*\d+\s*\]', 'warning', 'String Safety',
             'C-style string buffer', 'Use std::string'),

            (r'\bsprintf\s*\(', 'error', 'String Safety',
             'sprintf (unsafe)', 'Use std::format (C++20) or fmt::format'),

            (r'\bsnprintf\s*\(', 'warning', 'String Safety',
             'snprintf', 'Use std::format (C++20) or fmt::format'),

            (r'\bprintf\s*\(', 'info', 'I/O Modernization',
             'printf', 'Consider std::format with std::print (C++23) or std::cout'),

            (r'\bstrcpy\s*\(', 'error', 'String Safety',
             'strcpy (unsafe)', 'Use std::string'),

            (r'\bstrcat\s*\(', 'error', 'String Safety',
             'strcat (unsafe)', 'Use std::string'),

            # Casts
            (r'\(\s*\w+\s*\*?\s*\)\s*\w+', 'warning', 'Type Safety',
             'C-style cast', 'Use static_cast, dynamic_cast, or const_cast'),

            # Type definitions
            (r'\btypedef\s+', 'info', 'Type Declarations',
             'typedef', 'Use using = (alias declaration)'),

            # Loops (basic detection)
            (r'for\s*\(\s*\w+\s+\w+\s*=\s*0\s*;[^;]+;\s*\+\+', 'info', 'Loop Modernization',
             'Manual index loop', 'Consider range-based for or algorithms'),

            # Function pointers
            (r'\b\w+\s*\(\s*\*\s*\w+\s*\)\s*\(', 'info', 'Function Objects',
             'Raw function pointer', 'Use std::function or lambda'),

            # Manual locking
            (r'\.lock\s*\(\s*\)', 'warning', 'Concurrency',
             'Manual mutex lock', 'Use std::lock_guard or std::unique_lock'),

            # Bind (prefer lambdas)
            (r'std::bind\s*\(', 'info', 'Modern C++',
             'std::bind', 'Use lambda instead (clearer)'),
        ]

    def scan_file(self, file_path: Path) -> List[Issue]:
        """Scan a single C++ file for legacy patterns"""
        if not file_path.exists():
            print(f"Error: File not found: {file_path}", file=sys.stderr)
            return []

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Error reading {file_path}: {e}", file=sys.stderr)
            return []

        file_issues = []

        for line_num, line in enumerate(lines, start=1):
            # Skip comments (basic)
            if line.strip().startswith('//'):
                continue

            for pattern, severity, category, legacy, modern in self.patterns:
                if re.search(pattern, line):
                    issue = Issue(
                        file_path=str(file_path),
                        line_number=line_num,
                        category=category,
                        severity=severity,
                        legacy_pattern=legacy,
                        modern_alternative=modern,
                        code_snippet=line.strip()
                    )
                    file_issues.append(issue)
                    self.stats[category] += 1

        return file_issues

    def scan_directory(self, dir_path: Path) -> List[Issue]:
        """Recursively scan a directory for C++ files"""
        all_issues = []

        # C++ file extensions
        cpp_extensions = {'.cpp', '.cc', '.cxx', '.c', '.h', '.hpp', '.hxx'}

        for file_path in dir_path.rglob('*'):
            if file_path.is_file() and file_path.suffix in cpp_extensions:
                issues = self.scan_file(file_path)
                all_issues.extend(issues)

        return all_issues

    def format_results(self, issues: List[Issue]) -> str:
        """Format scan results for display"""
        if not issues:
            return "✓ No legacy C++ patterns detected! Code looks modern."

        output = []
        output.append(f"\n{'='*80}")
        output.append(f"Modern C++ Scanner Results - Found {len(issues)} issues")
        output.append(f"{'='*80}\n")

        # Group by category
        by_category = defaultdict(list)
        for issue in issues:
            by_category[issue.category].append(issue)

        # Display by category
        for category in sorted(by_category.keys()):
            cat_issues = by_category[category]
            output.append(f"\n## {category} ({len(cat_issues)} issues)")
            output.append("-" * 80)

            # Group by file within category
            by_file = defaultdict(list)
            for issue in cat_issues:
                by_file[issue.file_path].append(issue)

            for file_path in sorted(by_file.keys()):
                file_issues = by_file[file_path]
                output.append(f"\n  File: {file_path}")

                for issue in file_issues[:10 if not self.detailed else None]:
                    severity_icon = {
                        'error': '❌',
                        'warning': '⚠️ ',
                        'info': 'ℹ️ '
                    }.get(issue.severity, '•')

                    output.append(f"    {severity_icon} Line {issue.line_number}: {issue.legacy_pattern}")
                    output.append(f"       → {issue.modern_alternative}")
                    if self.detailed:
                        output.append(f"       Code: {issue.code_snippet}")
                    output.append("")

                if len(file_issues) > 10 and not self.detailed:
                    output.append(f"    ... and {len(file_issues) - 10} more issues")
                    output.append(f"    (use --detailed to see all)\n")

        # Summary statistics
        output.append(f"\n{'='*80}")
        output.append("Summary by Category:")
        output.append("-" * 80)
        for category, count in sorted(self.stats.items(), key=lambda x: x[1], reverse=True):
            output.append(f"  {category:30s}: {count:3d} issues")

        # Severity summary
        severity_counts = defaultdict(int)
        for issue in issues:
            severity_counts[issue.severity] += 1

        output.append("\nSummary by Severity:")
        output.append("-" * 80)
        for severity in ['error', 'warning', 'info']:
            if severity in severity_counts:
                icon = {'error': '❌', 'warning': '⚠️ ', 'info': 'ℹ️ '}[severity]
                output.append(f"  {icon} {severity.capitalize():10s}: {severity_counts[severity]:3d} issues")

        output.append(f"\n{'='*80}\n")

        return '\n'.join(output)

def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: python3 scan.py <file_or_directory> [--detailed]")
        print("\nScans C++ code for legacy patterns that should be modernized.")
        print("\nExamples:")
        print("  python3 scan.py src/main.cpp")
        print("  python3 scan.py src/ --detailed")
        sys.exit(1)

    path_arg = sys.argv[1]
    detailed = '--detailed' in sys.argv

    path = Path(path_arg)

    if not path.exists():
        print(f"Error: Path not found: {path}", file=sys.stderr)
        sys.exit(1)

    scanner = ModernCppScanner(detailed=detailed)

    print(f"Scanning {path}...")

    if path.is_file():
        issues = scanner.scan_file(path)
    else:
        issues = scanner.scan_directory(path)

    print(scanner.format_results(issues))

    # Exit code based on severity
    has_errors = any(issue.severity == 'error' for issue in issues)
    sys.exit(1 if has_errors else 0)

if __name__ == '__main__':
    main()
