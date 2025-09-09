#!/usr/bin/env python3
"""
Energy Drift Demonstration Script
=================================

This script creates a simple Monte Carlo simulation to demonstrate
how energy drift affects simulation results and statistical properties.

It compares:
1. A perfect Monte Carlo simulation (no energy drift)
2. A simulation with systematic energy drift
3. A simulation with random energy errors

This helps illustrate why energy drift checking is crucial in gRASPA.
"""

import argparse
import random
import math
from typing import List, Tuple

try:
    import numpy as np
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
    HAS_NUMPY = True
except ImportError:
    HAS_MATPLOTLIB = False
    HAS_NUMPY = False

class SimpleMonteCarloDemo:
    """Simple Monte Carlo simulation to demonstrate energy drift effects."""
    
    def __init__(self, n_steps: int = 10000, temperature: float = 1.0):
        self.n_steps = n_steps
        self.temperature = temperature
        self.kB = 1.0  # Boltzmann constant (reduced units)
        self.beta = 1.0 / (self.kB * temperature)
        
    def harmonic_potential(self, x: float) -> float:
        """Simple harmonic potential: V(x) = 0.5 * k * x^2"""
        k = 1.0  # Spring constant
        return 0.5 * k * x**2
    
    def perfect_mc_simulation(self) -> Tuple[List[float], List[float], List[float]]:
        """Run a perfect Monte Carlo simulation with no energy drift."""
        positions = [0.0]  # Start at origin
        energies = [self.harmonic_potential(0.0)]
        accepted_moves = []
        
        x = 0.0  # Current position
        n_accepted = 0
        
        for step in range(self.n_steps):
            # Propose new position
            x_new = x + random.gauss(0, 0.5)
            
            # Calculate energies
            E_old = self.harmonic_potential(x)
            E_new = self.harmonic_potential(x_new)
            
            # Accept/reject based on Metropolis criterion
            delta_E = E_new - E_old
            if delta_E < 0 or random.random() < math.exp(-self.beta * delta_E):
                x = x_new
                n_accepted += 1
                accepted_moves.append(1)
            else:
                accepted_moves.append(0)
            
            positions.append(x)
            energies.append(self.harmonic_potential(x))
        
        return positions, energies, accepted_moves
    
    def drift_affected_simulation(self, drift_rate: float = 1e-4) -> Tuple[List[float], List[float], List[float], List[float]]:
        """Run simulation with systematic energy drift."""
        positions = [0.0]
        true_energies = [self.harmonic_potential(0.0)]
        calculated_energies = [self.harmonic_potential(0.0)]  # What the simulation "thinks"
        accepted_moves = []
        
        x = 0.0
        energy_drift = 0.0  # Accumulated drift
        n_accepted = 0
        
        for step in range(self.n_steps):
            # Add systematic drift to energy calculations
            energy_drift += drift_rate * (step + 1)  # Drift grows with time
            
            # Propose new position
            x_new = x + random.gauss(0, 0.5)
            
            # True energies (what they should be)
            E_old_true = self.harmonic_potential(x)
            E_new_true = self.harmonic_potential(x_new)
            
            # Calculated energies (with drift)
            E_old_calc = E_old_true + energy_drift
            E_new_calc = E_new_true + energy_drift + drift_rate  # Drift accumulates
            
            # Accept/reject based on INCORRECT energies
            delta_E_calc = E_new_calc - E_old_calc
            if delta_E_calc < 0 or random.random() < math.exp(-self.beta * delta_E_calc):
                x = x_new
                n_accepted += 1
                accepted_moves.append(1)
            else:
                accepted_moves.append(0)
            
            positions.append(x)
            true_energies.append(E_new_true)
            calculated_energies.append(E_new_calc)
        
        return positions, true_energies, calculated_energies, accepted_moves
    
    def noisy_simulation(self, noise_level: float = 0.01) -> Tuple[List[float], List[float], List[float], List[float]]:
        """Run simulation with random energy errors (numerical noise)."""
        positions = [0.0]
        true_energies = [self.harmonic_potential(0.0)]
        calculated_energies = [self.harmonic_potential(0.0)]
        accepted_moves = []
        
        x = 0.0
        n_accepted = 0
        
        for step in range(self.n_steps):
            # Propose new position
            x_new = x + random.gauss(0, 0.5)
            
            # True energies
            E_old_true = self.harmonic_potential(x)
            E_new_true = self.harmonic_potential(x_new)
            
            # Add random noise to energy calculations
            E_old_calc = E_old_true + random.gauss(0, noise_level)
            E_new_calc = E_new_true + random.gauss(0, noise_level)
            
            # Accept/reject based on noisy energies
            delta_E_calc = E_new_calc - E_old_calc
            if delta_E_calc < 0 or random.random() < math.exp(-self.beta * delta_E_calc):
                x = x_new
                n_accepted += 1
                accepted_moves.append(1)
            else:
                accepted_moves.append(0)
            
            positions.append(x)
            true_energies.append(E_new_true)
            calculated_energies.append(E_new_calc)
        
        return positions, true_energies, calculated_energies, accepted_moves

def analyze_simulation_quality(positions: List[float], temperature: float = 1.0) -> dict:
    """Analyze the quality of a simulation by comparing to theoretical results."""
    # For harmonic oscillator at temperature T: <x^2> = kB*T/k = T (with k=1, kB=1)
    theoretical_variance = temperature
    
    # Calculate statistics from simulation (skip equilibration)
    positions_array = positions[1000:]  # Skip first 1000 steps
    mean_position = sum(positions_array) / len(positions_array)
    
    # Calculate variance manually
    variance = sum((x - mean_position)**2 for x in positions_array) / len(positions_array)
    
    return {
        'mean_position': mean_position,
        'variance': variance,
        'theoretical_variance': theoretical_variance,
        'variance_error': abs(variance - theoretical_variance) / theoretical_variance * 100
    }

def create_visualization(save_plots: bool = False):
    """Create visualization comparing different simulation scenarios."""
    if not HAS_MATPLOTLIB:
        print("Matplotlib not available. Running analysis without plots...")
        return run_analysis_only()
        
    demo = SimpleMonteCarloDemo(n_steps=10000, temperature=1.0)
    
    print("Running simulations...")
    
    # Run different simulations
    print("  1. Perfect simulation...")
    pos_perfect, eng_perfect, acc_perfect = demo.perfect_mc_simulation()
    
    print("  2. Simulation with energy drift...")
    pos_drift, eng_true_drift, eng_calc_drift, acc_drift = demo.drift_affected_simulation(drift_rate=1e-4)
    
    print("  3. Simulation with energy noise...")
    pos_noise, eng_true_noise, eng_calc_noise, acc_noise = demo.noisy_simulation(noise_level=0.01)
    
    # Analyze results
    analysis_perfect = analyze_simulation_quality(pos_perfect)
    analysis_drift = analyze_simulation_quality(pos_drift)
    analysis_noise = analyze_simulation_quality(pos_noise)
    
    # Create plots
    fig, axes = plt.subplots(2, 3, figsize=(15, 10))
    fig.suptitle('Effect of Energy Drift on Monte Carlo Simulations', fontsize=16)
    
    # Position traces
    steps = range(len(pos_perfect))
    axes[0, 0].plot(steps, pos_perfect, alpha=0.7, color='blue')
    axes[0, 0].set_title('Perfect Simulation\nPosition vs Time')
    axes[0, 0].set_ylabel('Position')
    
    axes[0, 1].plot(steps, pos_drift, alpha=0.7, color='red')
    axes[0, 1].set_title('With Energy Drift\nPosition vs Time')
    
    axes[0, 2].plot(steps, pos_noise, alpha=0.7, color='orange')
    axes[0, 2].set_title('With Energy Noise\nPosition vs Time')
    
    # Energy traces
    axes[1, 0].plot(steps, eng_perfect, alpha=0.7, color='blue')
    axes[1, 0].set_title('Perfect Simulation\nEnergy vs Time')
    axes[1, 0].set_xlabel('MC Steps')
    axes[1, 0].set_ylabel('Energy')
    
    # For drift simulation, show both true and calculated energies
    axes[1, 1].plot(steps, eng_true_drift, alpha=0.7, color='green', label='True Energy')
    axes[1, 1].plot(steps, eng_calc_drift, alpha=0.7, color='red', label='Calculated Energy')
    axes[1, 1].set_title('With Energy Drift\nEnergy vs Time')
    axes[1, 1].set_xlabel('MC Steps')
    axes[1, 1].legend()
    
    # For noise simulation
    axes[1, 2].plot(steps, eng_true_noise, alpha=0.7, color='green', label='True Energy')
    axes[1, 2].plot(steps, eng_calc_noise, alpha=0.5, color='orange', label='Calculated Energy')
    axes[1, 2].set_title('With Energy Noise\nEnergy vs Time')
    axes[1, 2].set_xlabel('MC Steps')
    axes[1, 2].legend()
    
    plt.tight_layout()
    
    if save_plots:
        plt.savefig('energy_drift_demonstration.png', dpi=300, bbox_inches='tight')
        print("Plot saved as 'energy_drift_demonstration.png'")
    else:
        plt.show()
    
    # Print analysis
    print("\n" + "="*60)
    print("SIMULATION QUALITY ANALYSIS")
    print("="*60)
    print(f"Theoretical variance for harmonic oscillator at T=1.0: {analysis_perfect['theoretical_variance']:.3f}")
    print()
    print(f"Perfect Simulation:")
    print(f"  Mean position: {analysis_perfect['mean_position']:.4f}")
    print(f"  Variance: {analysis_perfect['variance']:.4f}")
    print(f"  Error from theory: {analysis_perfect['variance_error']:.2f}%")
    print()
    print(f"Simulation with Energy Drift:")
    print(f"  Mean position: {analysis_drift['mean_position']:.4f}")
    print(f"  Variance: {analysis_drift['variance']:.4f}")
    print(f"  Error from theory: {analysis_drift['variance_error']:.2f}%")
    print()
    print(f"Simulation with Energy Noise:")
    print(f"  Mean position: {analysis_noise['mean_position']:.4f}")
    print(f"  Variance: {analysis_noise['variance']:.4f}")
    print(f"  Error from theory: {analysis_noise['variance_error']:.2f}%")
    print()
    print("KEY INSIGHTS:")
    print("• Perfect simulation reproduces theoretical results")
    print("• Energy drift can bias statistical properties")
    print("• Random energy errors increase sampling noise")
    print("• Both drift and noise reduce simulation quality")
    print("="*60)

def run_analysis_only():
    """Run analysis without plots when matplotlib is not available."""
    demo = SimpleMonteCarloDemo(n_steps=10000, temperature=1.0)
    
    print("Running simulations...")
    
    # Run different simulations
    print("  1. Perfect simulation...")
    pos_perfect, eng_perfect, acc_perfect = demo.perfect_mc_simulation()
    
    print("  2. Simulation with energy drift...")
    pos_drift, eng_true_drift, eng_calc_drift, acc_drift = demo.drift_affected_simulation(drift_rate=1e-4)
    
    print("  3. Simulation with energy noise...")
    pos_noise, eng_true_noise, eng_calc_noise, acc_noise = demo.noisy_simulation(noise_level=0.01)
    
    # Analyze results
    analysis_perfect = analyze_simulation_quality(pos_perfect)
    analysis_drift = analyze_simulation_quality(pos_drift)
    analysis_noise = analyze_simulation_quality(pos_noise)
    
    # Print analysis
    print("\n" + "="*60)
    print("SIMULATION QUALITY ANALYSIS")
    print("="*60)
    print(f"Theoretical variance for harmonic oscillator at T=1.0: {analysis_perfect['theoretical_variance']:.3f}")
    print()
    print(f"Perfect Simulation:")
    print(f"  Mean position: {analysis_perfect['mean_position']:.4f}")
    print(f"  Variance: {analysis_perfect['variance']:.4f}")
    print(f"  Error from theory: {analysis_perfect['variance_error']:.2f}%")
    print()
    print(f"Simulation with Energy Drift:")
    print(f"  Mean position: {analysis_drift['mean_position']:.4f}")
    print(f"  Variance: {analysis_drift['variance']:.4f}")
    print(f"  Error from theory: {analysis_drift['variance_error']:.2f}%")
    print()
    print(f"Simulation with Energy Noise:")
    print(f"  Mean position: {analysis_noise['mean_position']:.4f}")
    print(f"  Variance: {analysis_noise['variance']:.4f}")
    print(f"  Error from theory: {analysis_noise['variance_error']:.2f}%")
    print()
    print("KEY INSIGHTS:")
    print("• Perfect simulation reproduces theoretical results")
    print("• Energy drift can bias statistical properties")
    print("• Random energy errors increase sampling noise")
    print("• Both drift and noise reduce simulation quality")
    print("• gRASPA's energy drift checking prevents these issues")
    print("="*60)

def main():
    parser = argparse.ArgumentParser(description='Demonstrate energy drift effects in Monte Carlo simulation')
    parser.add_argument('--save-plots', action='store_true', help='Save plots to file instead of displaying')
    
    args = parser.parse_args()
    
    try:
        create_visualization(save_plots=args.save_plots)
    except Exception as e:
        print(f"Error running visualization: {e}")
        print("\nRunning analysis without plots...")
        run_analysis_only()

if __name__ == "__main__":
    main()