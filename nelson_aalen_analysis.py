#!/usr/bin/env python3
"""
Nelson-Aalen Estimation for SGF State Transitions

This script reads the SGF (Solo, Grupo, Fermo) transition data and performs
Nelson-Aalen estimation of cumulative hazard functions for transitions between states.

The Nelson-Aalen estimator provides non-parametric estimates of the cumulative hazard
function, which can be used to understand transition rates between different states.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
from collections import defaultdict
import warnings
warnings.filterwarnings('ignore')

class NelsonAalenEstimator:
    """
    Nelson-Aalen estimator for cumulative hazard function.
    """
    
    def __init__(self):
        self.times = None
        self.cumulative_hazard = None
        self.at_risk = None
        self.events = None
    
    def fit(self, durations, events, weights=None):
        """
        Fit the Nelson-Aalen estimator.
        
        Parameters:
        -----------
        durations : array-like
            Duration times until event or censoring
        events : array-like
            Event indicator (1 if event occurred, 0 if censored)
        weights : array-like, optional
            Weights for each observation
        """
        durations = np.array(durations)
        events = np.array(events)
        
        if weights is None:
            weights = np.ones(len(durations))
        else:
            weights = np.array(weights)
        
        # Get unique event times
        unique_times = np.unique(durations[events == 1])
        unique_times = np.sort(unique_times)
        
        cumulative_hazard = np.zeros(len(unique_times))
        at_risk_counts = np.zeros(len(unique_times))
        event_counts = np.zeros(len(unique_times))
        
        for i, t in enumerate(unique_times):
            # Number at risk at time t (those with duration >= t)
            at_risk = np.sum(weights[durations >= t])
            
            # Number of events at time t
            events_at_t = np.sum(weights[(durations == t) & (events == 1)])
            
            at_risk_counts[i] = at_risk
            event_counts[i] = events_at_t
            
            # Nelson-Aalen increment: events / at_risk
            if at_risk > 0:
                hazard_increment = events_at_t / at_risk
                if i == 0:
                    cumulative_hazard[i] = hazard_increment
                else:
                    cumulative_hazard[i] = cumulative_hazard[i-1] + hazard_increment
        
        self.times = unique_times
        self.cumulative_hazard = cumulative_hazard
        self.at_risk = at_risk_counts
        self.events = event_counts
        
        return self
    
    def plot(self, ax=None, label=None, **kwargs):
        """Plot the cumulative hazard function."""
        if ax is None:
            fig, ax = plt.subplots(figsize=(10, 6))
        
        # Create step function
        times_extended = np.concatenate([[0], self.times])
        hazard_extended = np.concatenate([[0], self.cumulative_hazard])
        
        ax.step(times_extended, hazard_extended, where='post', label=label, **kwargs)
        ax.set_xlabel('Time')
        ax.set_ylabel('Cumulative Hazard')
        
        return ax

def load_sgf_data(data_dir):
    """
    Load SGF transition data from CSV files.
    
    Parameters:
    -----------
    data_dir : str or Path
        Directory containing the CSV files
    
    Returns:
    --------
    survival_data : pandas.DataFrame
        Survival data with transitions
    state_definitions : pandas.DataFrame
        State definitions mapping indices to (s, g, f) triples
    """
    data_dir = Path(data_dir)
    
    # Load survival data
    survival_file = data_dir / "sgf_survival_data.csv"
    survival_data = pd.read_csv(survival_file)
    
    # Load state definitions
    triples_file = data_dir / "sgf_triples.csv"
    state_definitions = pd.read_csv(triples_file)
    
    print(f"Loaded {len(survival_data)} transition records")
    print(f"Found {len(state_definitions)} possible states")
    
    return survival_data, state_definitions

def analyze_transitions_by_state(survival_data, state_definitions):
    """
    Perform Nelson-Aalen analysis for transitions from each state.
    
    Parameters:
    -----------
    survival_data : pandas.DataFrame
        Survival data with transitions
    state_definitions : pandas.DataFrame
        State definitions
    
    Returns:
    --------
    results : dict
        Dictionary containing Nelson-Aalen estimators for each state
    """
    results = {}
    
    # Group by from_state
    for from_idx in survival_data['from_index'].unique():
        state_data = survival_data[survival_data['from_index'] == from_idx].copy()
        
        if len(state_data) == 0:
            continue
        
        # Get state information
        state_info = state_definitions[state_definitions['index'] == from_idx].iloc[0]
        state_name = f"({state_info['solo']}, {state_info['grupo']}, {state_info['fermo']})"
        
        # Fit Nelson-Aalen estimator
        # Note: censored=1 means censored, censored=0 means event occurred
        events = 1 - state_data['censored'].values  # Convert to event indicator
        durations = state_data['duration'].values
        
        na_estimator = NelsonAalenEstimator()
        na_estimator.fit(durations, events)
        
        results[from_idx] = {
            'estimator': na_estimator,
            'state_name': state_name,
            'n_transitions': len(state_data),
            'n_events': np.sum(events),
            'n_censored': np.sum(state_data['censored']),
            'state_info': state_info
        }
        
        print(f"State {from_idx} {state_name}: {len(state_data)} observations, "
              f"{np.sum(events)} events, {np.sum(state_data['censored'])} censored")
    
    return results

def analyze_specific_transitions(survival_data, state_definitions):
    """
    Perform Nelson-Aalen analysis for specific state-to-state transitions.
    """
    transition_results = {}
    
    # Group by from_index and to_index
    transition_groups = survival_data.groupby(['from_index', 'to_index'])
    
    for (from_idx, to_idx), group in transition_groups:
        if len(group) < 5:  # Skip transitions with very few observations
            continue
        
        # Get state names
        from_state = state_definitions[state_definitions['index'] == from_idx].iloc[0]
        to_state = state_definitions[state_definitions['index'] == to_idx].iloc[0]
        
        from_name = f"({from_state['solo']}, {from_state['grupo']}, {from_state['fermo']})"
        to_name = f"({to_state['solo']}, {to_state['grupo']}, {to_state['fermo']})"
        transition_name = f"{from_name} → {to_name}"
        
        # Fit Nelson-Aalen estimator
        events = 1 - group['censored'].values
        durations = group['duration'].values
        
        na_estimator = NelsonAalenEstimator()
        na_estimator.fit(durations, events)
        
        transition_results[(from_idx, to_idx)] = {
            'estimator': na_estimator,
            'transition_name': transition_name,
            'n_transitions': len(group),
            'n_events': np.sum(events),
            'n_censored': np.sum(group['censored'])
        }
    
    return transition_results

def plot_cumulative_hazards_by_state(results, max_states=10):
    """
    Plot cumulative hazard functions for transitions from different states.
    """
    fig, ax = plt.subplots(figsize=(12, 8))
    
    # Sort by number of transitions and plot top states
    sorted_states = sorted(results.items(), 
                          key=lambda x: x[1]['n_transitions'], 
                          reverse=True)[:max_states]
    
    colors = plt.cm.tab10(np.linspace(0, 1, len(sorted_states)))
    
    for i, (state_idx, result) in enumerate(sorted_states):
        result['estimator'].plot(ax=ax, 
                               label=f"State {state_idx} {result['state_name']} (n={result['n_transitions']})",
                               color=colors[i])
    
    ax.set_title('Nelson-Aalen Cumulative Hazard Estimates by Starting State')
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig

def plot_transition_specific_hazards(transition_results, max_transitions=15):
    """
    Plot cumulative hazard functions for specific transitions.
    """
    fig, ax = plt.subplots(figsize=(14, 8))
    
    # Sort by number of transitions
    sorted_transitions = sorted(transition_results.items(), 
                              key=lambda x: x[1]['n_transitions'], 
                              reverse=True)[:max_transitions]
    
    colors = plt.cm.tab20(np.linspace(0, 1, len(sorted_transitions)))
    
    for i, ((from_idx, to_idx), result) in enumerate(sorted_transitions):
        result['estimator'].plot(ax=ax, 
                               label=f"{result['transition_name']} (n={result['n_transitions']})",
                               color=colors[i])
    
    ax.set_title('Nelson-Aalen Cumulative Hazard Estimates for Specific Transitions')
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=8)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig

def create_transition_summary_table(results):
    """
    Create a summary table of transition statistics.
    """
    summary_data = []
    
    for state_idx, result in results.items():
        summary_data.append({
            'State_Index': state_idx,
            'State_SGF': result['state_name'],
            'Solo': result['state_info']['solo'],
            'Grupo': result['state_info']['grupo'],
            'Fermo': result['state_info']['fermo'],
            'Total_Observations': result['n_transitions'],
            'Events': result['n_events'],
            'Censored': result['n_censored'],
            'Event_Rate': result['n_events'] / result['n_transitions'] if result['n_transitions'] > 0 else 0,
            'Final_Cumulative_Hazard': result['estimator'].cumulative_hazard[-1] if len(result['estimator'].cumulative_hazard) > 0 else 0
        })
    
    df = pd.DataFrame(summary_data)
    df = df.sort_values('Total_Observations', ascending=False)
    
    return df

def main():
    """
    Main function to run the Nelson-Aalen analysis.
    """
    print("Nelson-Aalen Analysis of SGF State Transitions")
    print("=" * 50)
    
    # Load data
    data_dir = "cwaggle/bin"  # Adjust path as needed
    survival_data, state_definitions = load_sgf_data(data_dir)
    
    print(f"\nData Overview:")
    print(f"- Total transitions: {len(survival_data)}")
    print(f"- Unique from states: {survival_data['from_index'].nunique()}")
    print(f"- Unique to states: {survival_data['to_index'].nunique()}")
    print(f"- Censoring rate: {survival_data['censored'].mean():.3f}")
    
    # Analyze transitions by starting state
    print("\nAnalyzing transitions by starting state...")
    state_results = analyze_transitions_by_state(survival_data, state_definitions)
    
    # Analyze specific state-to-state transitions
    print("\nAnalyzing specific state-to-state transitions...")
    transition_results = analyze_specific_transitions(survival_data, state_definitions)
    
    # Create summary table
    summary_table = create_transition_summary_table(state_results)
    print("\nTransition Summary (Top 15 states by observation count):")
    print(summary_table.head(15).to_string(index=False))
    
    # Create plots
    print("\nGenerating plots...")
    
    # Plot 1: Cumulative hazards by state
    fig1 = plot_cumulative_hazards_by_state(state_results)
    fig1.savefig('nelson_aalen_by_state.png', dpi=300, bbox_inches='tight')
    print("Saved: nelson_aalen_by_state.png")
    
    # Plot 2: Specific transitions
    fig2 = plot_transition_specific_hazards(transition_results)
    fig2.savefig('nelson_aalen_specific_transitions.png', dpi=300, bbox_inches='tight')
    print("Saved: nelson_aalen_specific_transitions.png")
    
    # Save summary table
    summary_table.to_csv('sgf_transition_summary_table.csv', index=False)
    print("Saved: sgf_transition_summary_table.csv")
    
    # Print some key insights
    print("\nKey Insights:")
    print(f"- Most active state: {summary_table.iloc[0]['State_SGF']} with {summary_table.iloc[0]['Total_Observations']} transitions")
    print(f"- Highest event rate: {summary_table.loc[summary_table['Event_Rate'].idxmax(), 'State_SGF']} ({summary_table['Event_Rate'].max():.3f})")
    print(f"- Highest cumulative hazard: {summary_table.loc[summary_table['Final_Cumulative_Hazard'].idxmax(), 'State_SGF']} ({summary_table['Final_Cumulative_Hazard'].max():.3f})")
    
    plt.show()

if __name__ == "__main__":
    main()
