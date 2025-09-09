#!/usr/bin/env python3
"""
Energy Drift Analysis Tool for gRASPA
=====================================

This script analyzes energy drift from gRASPA simulation output files and provides
insights into simulation quality and potential issues.

Usage:
    python analyze_energy_drift.py <output_file>

Example:
    python analyze_energy_drift.py Examples/CO2-MFI/output.txt
"""

import re
import sys
import argparse
import os
from typing import Dict, List, Tuple, Optional

class EnergyDriftAnalyzer:
    """Analyzes energy drift patterns in gRASPA simulation output."""
    
    def __init__(self, output_file: str):
        self.output_file = output_file
        self.energy_sections = {}
        self.dnn_drift_data = {}
        self.dnn_rejection_data = {}
        
    def parse_output_file(self) -> bool:
        """Parse the gRASPA output file and extract energy drift information."""
        try:
            with open(self.output_file, 'r') as f:
                content = f.read()
            
            # Find energy drift sections
            self._extract_energy_drift_sections(content)
            
            # Find DNN drift summary
            self._extract_dnn_drift_summary(content)
            
            # Find DNN rejection summary
            self._extract_dnn_rejection_summary(content)
            
            return True
            
        except FileNotFoundError:
            print(f"Error: File '{self.output_file}' not found.")
            return False
        except Exception as e:
            print(f"Error parsing file: {e}")
            return False
    
    def _extract_energy_drift_sections(self, content: str):
        """Extract energy drift and GPU drift sections."""
        # Pattern to match energy drift sections with more flexible matching
        drift_pattern = re.compile(r'\*\*\* (ENERGY DRIFT[^*]*?) \*\*\*.*?Total Energy:\s+([-+]?[0-9]*\.?[0-9]+)', re.DOTALL)
        gpu_drift_pattern = re.compile(r'\*\*\* (GPU DRIFT[^*]*?) \*\*\*.*?Total Energy:\s+([-+]?[0-9]*\.?[0-9]+)', re.DOTALL)
        
        # Find CPU energy drift
        matches = drift_pattern.findall(content)
        for section_type, energy_value in matches:
            self.energy_sections['CPU_DRIFT'] = float(energy_value)
        
        # Find GPU drift
        gpu_matches = gpu_drift_pattern.findall(content)
        for section_type, energy_value in gpu_matches:
            self.energy_sections['GPU_DRIFT'] = float(energy_value)
    
    def _extract_dnn_drift_summary(self, content: str):
        """Extract DNN drift summary data."""
        dnn_drift_pattern = re.compile(
            r'DNN Drift Summary:\s*\n'
            r'Translation\+Rotation:\s+([-+]?[0-9]*\.?[0-9]+)\s*\n'
            r'Reinsertion:\s+([-+]?[0-9]*\.?[0-9]+)\s*\n'
            r'Insertion:\s+([-+]?[0-9]*\.?[0-9]+)\s*\n'
            r'Deletion:\s+([-+]?[0-9]*\.?[0-9]+)\s*\n'
            r'SingleSwap:\s+([-+]?[0-9]*\.?[0-9]+)'
        )
        
        match = dnn_drift_pattern.search(content)
        if match:
            self.dnn_drift_data = {
                'translation_rotation': float(match.group(1)),
                'reinsertion': float(match.group(2)),
                'insertion': float(match.group(3)),
                'deletion': float(match.group(4)),
                'single_swap': float(match.group(5))
            }
    
    def _extract_dnn_rejection_summary(self, content: str):
        """Extract DNN rejection summary data."""
        dnn_rejection_pattern = re.compile(
            r'DNN Rejection Summary:\s*\n'
            r'Translation\+Rotation:\s+(\d+)\s*\n'
            r'Reinsertion:\s+(\d+)\s*\n'
            r'Insertion:\s+(\d+)\s*\n'
            r'Deletion:\s+(\d+)\s*\n'
            r'SingleSwap:\s+(\d+)'
        )
        
        match = dnn_rejection_pattern.search(content)
        if match:
            self.dnn_rejection_data = {
                'translation_rotation': int(match.group(1)),
                'reinsertion': int(match.group(2)),
                'insertion': int(match.group(3)),
                'deletion': int(match.group(4)),
                'single_swap': int(match.group(5))
            }
    
    def analyze_energy_drift(self) -> Dict:
        """Analyze the energy drift data and provide assessment."""
        analysis = {
            'overall_quality': 'Unknown',
            'issues': [],
            'recommendations': [],
            'drift_values': self.energy_sections.copy()
        }
        
        # Threshold for acceptable energy drift (in internal units)
        ACCEPTABLE_DRIFT = 1e-3
        WARNING_DRIFT = 1e-2
        CRITICAL_DRIFT = 1e-1
        
        # Analyze CPU energy drift
        if 'CPU_DRIFT' in self.energy_sections:
            cpu_drift = abs(self.energy_sections['CPU_DRIFT'])
            
            if cpu_drift < ACCEPTABLE_DRIFT:
                analysis['overall_quality'] = 'Excellent'
            elif cpu_drift < WARNING_DRIFT:
                analysis['overall_quality'] = 'Good'
                analysis['issues'].append(f"Minor CPU energy drift detected: {cpu_drift:.2e}")
                analysis['recommendations'].append("Monitor energy drift in longer simulations")
            elif cpu_drift < CRITICAL_DRIFT:
                analysis['overall_quality'] = 'Concerning'
                analysis['issues'].append(f"Significant CPU energy drift: {cpu_drift:.2e}")
                analysis['recommendations'].append("Check simulation parameters and force field settings")
            else:
                analysis['overall_quality'] = 'Poor'
                analysis['issues'].append(f"Critical CPU energy drift: {cpu_drift:.2e}")
                analysis['recommendations'].append("Simulation results may be unreliable - investigate immediately")
        
        # Analyze GPU drift
        if 'GPU_DRIFT' in self.energy_sections:
            gpu_drift = abs(self.energy_sections['GPU_DRIFT'])
            
            if gpu_drift > WARNING_DRIFT:
                analysis['issues'].append(f"GPU-CPU energy mismatch: {gpu_drift:.2e}")
                analysis['recommendations'].append("Check GPU precision settings or use CPU-only calculation")
        
        return analysis
    
    def analyze_dnn_performance(self) -> Dict:
        """Analyze DNN performance based on drift and rejection data."""
        if not self.dnn_drift_data and not self.dnn_rejection_data:
            return {'status': 'No DNN data found'}
        
        analysis = {
            'total_drift': 0,
            'total_rejections': 0,
            'move_type_analysis': {},
            'recommendations': []
        }
        
        # Calculate total drift and rejections
        if self.dnn_drift_data:
            analysis['total_drift'] = sum(self.dnn_drift_data.values())
            
        if self.dnn_rejection_data:
            analysis['total_rejections'] = sum(self.dnn_rejection_data.values())
        
        # Analyze by move type
        move_types = ['translation_rotation', 'reinsertion', 'insertion', 'deletion', 'single_swap']
        
        for move_type in move_types:
            drift = self.dnn_drift_data.get(move_type, 0)
            rejections = self.dnn_rejection_data.get(move_type, 0)
            
            analysis['move_type_analysis'][move_type] = {
                'drift': drift,
                'rejections': rejections,
                'quality': 'Good'  # Default
            }
            
            # Assess quality based on rejections
            if rejections > 1000:
                analysis['move_type_analysis'][move_type]['quality'] = 'Poor'
                analysis['recommendations'].append(
                    f"High rejection rate for {move_type.replace('_', ' ')}: {rejections} rejections"
                )
            elif rejections > 100:
                analysis['move_type_analysis'][move_type]['quality'] = 'Fair'
        
        # Overall DNN assessment
        if analysis['total_rejections'] > 5000:
            analysis['recommendations'].append("Consider retraining DNN model or increasing MaxDNNDrift threshold")
        elif analysis['total_rejections'] > 1000:
            analysis['recommendations'].append("Monitor DNN performance - moderate rejection rate")
        
        return analysis
    
    def generate_report(self) -> str:
        """Generate a comprehensive analysis report."""
        if not self.parse_output_file():
            return "Failed to parse output file."
        
        report = []
        report.append("=" * 60)
        report.append(f"Energy Drift Analysis Report")
        report.append(f"File: {os.path.basename(self.output_file)}")
        report.append("=" * 60)
        
        # Energy drift analysis
        drift_analysis = self.analyze_energy_drift()
        report.append(f"\n📊 ENERGY DRIFT ASSESSMENT")
        report.append(f"Overall Quality: {drift_analysis['overall_quality']}")
        
        if drift_analysis['drift_values']:
            report.append(f"\nEnergy Drift Values (internal units):")
            for drift_type, value in drift_analysis['drift_values'].items():
                report.append(f"  {drift_type}: {value:.2e}")
        
        if drift_analysis['issues']:
            report.append(f"\n⚠️ ISSUES DETECTED:")
            for issue in drift_analysis['issues']:
                report.append(f"  • {issue}")
        
        if drift_analysis['recommendations']:
            report.append(f"\n💡 RECOMMENDATIONS:")
            for rec in drift_analysis['recommendations']:
                report.append(f"  • {rec}")
        
        # DNN analysis
        dnn_analysis = self.analyze_dnn_performance()
        if dnn_analysis.get('status') != 'No DNN data found':
            report.append(f"\n🤖 DEEP NEURAL NETWORK ANALYSIS")
            report.append(f"Total DNN Drift: {dnn_analysis['total_drift']:.2f}")
            report.append(f"Total DNN Rejections: {dnn_analysis['total_rejections']}")
            
            report.append(f"\nMove Type Breakdown:")
            for move_type, data in dnn_analysis['move_type_analysis'].items():
                quality_emoji = {"Good": "✅", "Fair": "⚠️", "Poor": "❌"}
                emoji = quality_emoji.get(data['quality'], "❓")
                report.append(f"  {emoji} {move_type.replace('_', ' ').title()}: "
                            f"{data['drift']:.2f} drift, {data['rejections']} rejections")
            
            if dnn_analysis['recommendations']:
                report.append(f"\n💡 DNN RECOMMENDATIONS:")
                for rec in dnn_analysis['recommendations']:
                    report.append(f"  • {rec}")
        
        report.append(f"\n" + "=" * 60)
        report.append(f"Analysis complete. For more information, see ENERGY_DRIFT_EXPLAINED.md")
        report.append("=" * 60)
        
        return "\n".join(report)

def main():
    parser = argparse.ArgumentParser(description='Analyze energy drift in gRASPA simulation output')
    parser.add_argument('output_file', help='Path to gRASPA output file')
    
    args = parser.parse_args()
    
    analyzer = EnergyDriftAnalyzer(args.output_file)
    report = analyzer.generate_report()
    print(report)

if __name__ == "__main__":
    main()