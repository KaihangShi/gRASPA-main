# Energy Drift Analysis Tools for gRASPA

This directory contains educational tools and documentation about energy drift in molecular simulations, specifically for gRASPA users.

## Files Overview

### Documentation
- **`ENERGY_DRIFT_EXPLAINED.md`** - Comprehensive explanation of energy drift, why it matters, and how it affects simulation results

### Analysis Tools
- **`analyze_energy_drift.py`** - Python script to analyze energy drift from gRASPA output files
- **`demo_energy_drift_effects.py`** - Educational demonstration showing how energy drift affects Monte Carlo simulations

## Quick Start

### Analyze Your Simulation Results
```bash
python3 analyze_energy_drift.py path/to/your/output.txt
```

This will provide:
- Energy drift assessment (CPU drift, GPU drift)
- DNN drift analysis (if using machine learning potentials)
- Quality recommendations
- Identification of potential issues

### Understand Energy Drift Effects
```bash
python3 demo_energy_drift_effects.py
```

This runs educational Monte Carlo simulations showing:
- Perfect simulation (no energy drift)
- Simulation with systematic energy drift
- Simulation with random energy noise
- Comparison of statistical accuracy

## Key Concepts

### Why Energy Drift Matters
Energy drift in molecular simulations can:
- Corrupt statistical ensemble sampling
- Lead to incorrect thermodynamic properties
- Cause unphysical molecular configurations
- Reduce simulation reliability and reproducibility

### Types of Energy Drift in gRASPA
1. **Total Energy Drift**: Mismatch between running energy tracking and final calculated energy
2. **GPU Drift**: Differences between GPU and CPU energy calculations  
3. **DNN Drift**: Discrepancies between machine learning and classical force field predictions

### Energy Drift Thresholds
- **Acceptable**: < 1e-3 (internal units)
- **Warning**: 1e-3 to 1e-2
- **Critical**: > 1e-1

### Best Practices
1. Always monitor energy drift in simulation output
2. Adjust `MaxDNNDrift` parameter for ML potential simulations
3. Validate GPU calculations against CPU results
4. Compare ML predictions with classical force fields
5. Investigate any systematic energy drift trends

## Example Usage

### Basic Energy Drift Analysis
```bash
# Analyze a completed simulation
python3 analyze_energy_drift.py Examples/CO2-MFI/output.txt

# Check DNN performance in ML potential simulation
python3 analyze_energy_drift.py Examples/CO2_MgMOF74_Allegro/output.txt
```

### Understanding the Impact
```bash
# See how energy drift affects simulation quality
python3 demo_energy_drift_effects.py

# Save visualization plots (requires matplotlib)
python3 demo_energy_drift_effects.py --save-plots
```

## Troubleshooting Energy Drift Issues

### High CPU Energy Drift
- Check force field parameters
- Verify simulation input consistency  
- Consider reducing time step or move sizes
- Examine long-range interaction settings

### High GPU Drift
- Verify GPU precision settings
- Compare with CPU-only calculations
- Check for hardware issues
- Consider using double precision

### High DNN Drift
- Increase `MaxDNNDrift` threshold if appropriate
- Retrain ML model with more diverse data
- Validate ML model performance
- Consider hybrid ML/classical approaches

## Integration with gRASPA Workflow

These tools complement gRASPA's built-in energy drift monitoring:

1. **During Simulation**: gRASPA automatically checks and reports energy drift
2. **After Simulation**: Use `analyze_energy_drift.py` for detailed assessment
3. **Education/Debugging**: Use `demo_energy_drift_effects.py` to understand concepts
4. **Documentation**: Refer to `ENERGY_DRIFT_EXPLAINED.md` for detailed explanations

## Requirements

- Python 3.6+
- No additional dependencies for basic functionality
- Optional: matplotlib and numpy for visualization in demo script

## Further Reading

- gRASPA User Manual: https://zhaoli2042.github.io/gRASPA-mkdoc
- Original gRASPA paper (in preparation)
- Monte Carlo simulation theory and best practices