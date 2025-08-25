# Nelson-Aalen Analysis for SGF State Transitions

This repository contains Python scripts for performing Nelson-Aalen estimation of transition rates between SGF (Solo, Grupo, Fermo) states.

## Generated Scripts

### 1. `nelson_aalen_simple.py`
A straightforward implementation of Nelson-Aalen estimation with basic visualization.

**Features:**
- Manual implementation of Nelson-Aalen estimator
- Analysis by starting state
- Basic transition type analysis
- Cumulative hazard plots

**Outputs:**
- `nelson_aalen_sgf_analysis.png` - Cumulative hazard curves
- `nelson_aalen_summary.csv` - Summary statistics

### 2. `nelson_aalen_advanced.py`
Comprehensive analysis with multiple perspectives on the transition data.

**Features:**
- Empirical hazard rate calculations
- Analysis by SGF components
- Transition pattern classification
- Multi-panel visualization

**Outputs:**
- `detailed_sgf_analysis.csv` - Detailed state analysis
- `sgf_component_analysis.csv` - Analysis by SGF values
- `transition_pattern_hazards.csv` - Hazard rates by transition type
- `sgf_hazard_analysis.png` - Multi-panel hazard analysis plots

### 3. `nelson_aalen_analysis.py`
Full-featured implementation with object-oriented design (alternative implementation).

## Data Requirements

The scripts expect to find these CSV files in the `cwaggle/bin/` directory:
- `sgf_survival_data.csv` - Transition data with durations and censoring
- `sgf_triples.csv` - State definitions mapping indices to (solo, grupo, fermo) triples

## Usage

```bash
# Set up Python environment
python -m venv .venv
source .venv/bin/activate  # On macOS/Linux
# .venv\Scripts\activate   # On Windows

# Install dependencies
pip install pandas numpy matplotlib seaborn

# Run analysis
python nelson_aalen_simple.py      # Basic analysis
python nelson_aalen_advanced.py    # Comprehensive analysis
```

## Key Results

The analysis reveals:

1. **State Activity**: States with balanced SGF distributions (e.g., (3,4,5), (3,5,4)) show highest transition frequencies
2. **Transition Types**: 
   - Complex transitions have highest hazard rates (0.173)
   - →Fermo transitions are common (0.143 hazard rate)
   - Solo↔Grupo transitions have moderate rates
3. **Event Rates**: Nearly 100% event rate indicates very low censoring
4. **Patterns**: Most transitions involve single robot state changes between Solo, Grupo, and Fermo states

## Nelson-Aalen Estimator

The Nelson-Aalen estimator provides a non-parametric estimate of the cumulative hazard function:

```
Λ(t) = Σ(dᵢ/nᵢ)
```

Where:
- `dᵢ` = number of events at time `tᵢ`  
- `nᵢ` = number at risk at time `tᵢ`
- Sum is over all event times ≤ t

This estimator is particularly useful for:
- Understanding transition intensity over time
- Comparing hazard rates between different states
- Identifying patterns in state evolution

## Interpretation

- **Cumulative Hazard**: Higher values indicate states that transition more frequently
- **Empirical Hazard Rate**: Events per unit time, useful for comparing state stability
- **Event Rate**: Proportion of uncensored transitions (completion rate)

The SGF states represent robot behaviors:
- **Solo**: Individual foraging
- **Grupo**: Collective behavior  
- **Fermo**: Stationary/inactive state
