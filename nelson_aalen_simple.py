#!/usr/bin/env python3
"""
Simplified Nelson-Aalen Analysis for SGF Transitions

This script provides a focused implementation of Nelson-Aalen estimation
for the SGF (Solo, Grupo, Fermo) state transition data.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

def nelson_aalen_estimator(durations, events):
    """
    Calculate Nelson-Aalen cumulative hazard estimate.
    
    Parameters:
    -----------
    durations : array-like
        Time until event or censoring
    events : array-like 
        Event indicator (1 = event, 0 = censored)
    
    Returns:
    --------
    times : numpy.array
        Unique event times
    cumulative_hazard : numpy.array
        Cumulative hazard at each time
    """
    durations = np.array(durations)
    events = np.array(events)
    
    # Get unique event times (only when events occur)
    event_mask = events == 1
    unique_times = np.unique(durations[event_mask])
    unique_times = np.sort(unique_times)
    
    cumulative_hazard = np.zeros(len(unique_times))
    
    for i, t in enumerate(unique_times):
        # Number at risk at time t
        at_risk = np.sum(durations >= t)
        
        # Number of events at exactly time t
        events_at_t = np.sum((durations == t) & (events == 1))
        
        # Nelson-Aalen increment
        if at_risk > 0:
            hazard_increment = events_at_t / at_risk
            if i == 0:
                cumulative_hazard[i] = hazard_increment
            else:
                cumulative_hazard[i] = cumulative_hazard[i-1] + hazard_increment
    
    return unique_times, cumulative_hazard

def load_and_analyze_sgf_data():
    """
    Load SGF data and perform Nelson-Aalen analysis.
    """
    # Load the data
    try:
        survival_data = pd.read_csv('cwaggle/bin/sgf_survival_data.csv')
        state_definitions = pd.read_csv('cwaggle/bin/sgf_triples.csv')
    except FileNotFoundError:
        print("Error: CSV files not found. Make sure you're running from the correct directory.")
        return None, None
    
    print(f"Loaded {len(survival_data)} transition records")
    print(f"Censoring rate: {survival_data['censored'].mean():.1%}")
    
    # Convert censored indicator to event indicator
    # In the data: censored=1 means censored, censored=0 means event occurred
    survival_data['event'] = 1 - survival_data['censored']
    
    return survival_data, state_definitions

def analyze_by_starting_state(survival_data, state_definitions):
    """
    Perform Nelson-Aalen analysis grouped by starting state.
    """
    results = {}
    
    # Get the most common starting states
    state_counts = survival_data['from_index'].value_counts()
    print(f"\nTop 10 starting states by frequency:")
    
    for i, (state_idx, count) in enumerate(state_counts.head(10).items()):
        # Get state definition
        state_row = state_definitions[state_definitions['index'] == state_idx]
        if len(state_row) == 0:
            continue
        
        state_info = state_row.iloc[0]
        state_name = f"({state_info['solo']}, {state_info['grupo']}, {state_info['fermo']})"
        
        # Get data for this state
        state_data = survival_data[survival_data['from_index'] == state_idx]
        
        # Calculate Nelson-Aalen estimate
        times, cum_hazard = nelson_aalen_estimator(
            state_data['duration'].values,
            state_data['event'].values
        )
        
        results[state_idx] = {
            'times': times,
            'cumulative_hazard': cum_hazard,
            'state_name': state_name,
            'n_obs': len(state_data),
            'n_events': state_data['event'].sum(),
            'final_hazard': cum_hazard[-1] if len(cum_hazard) > 0 else 0
        }
        
        print(f"{i+1:2d}. State {state_idx:2d} {state_name}: {count:4d} transitions, "
              f"{state_data['event'].sum():4d} events, "
              f"final hazard: {cum_hazard[-1] if len(cum_hazard) > 0 else 0:.3f}")
    
    return results

def plot_nelson_aalen_curves(results, max_curves=8):
    """
    Plot Nelson-Aalen cumulative hazard curves.
    """
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Sort by number of observations for consistent plotting
    sorted_results = sorted(results.items(), 
                           key=lambda x: x[1]['n_obs'], 
                           reverse=True)[:max_curves]
    
    colors = plt.cm.tab10(np.linspace(0, 1, len(sorted_results)))
    
    # Plot 1: Cumulative hazard curves
    for i, (state_idx, result) in enumerate(sorted_results):
        if len(result['times']) > 0:
            # Create step function for proper visualization
            times_step = np.concatenate([[0], result['times']])
            hazard_step = np.concatenate([[0], result['cumulative_hazard']])
            
            ax1.step(times_step, hazard_step, where='post', 
                    label=f"State {state_idx} {result['state_name']}", 
                    color=colors[i], linewidth=2)
    
    ax1.set_xlabel('Time')
    ax1.set_ylabel('Cumulative Hazard')
    ax1.set_title('Nelson-Aalen Cumulative Hazard Estimates')
    ax1.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=9)
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Final cumulative hazard values
    states = [f"S{idx}\n{res['state_name']}" for idx, res in sorted_results]
    final_hazards = [res['final_hazard'] for _, res in sorted_results]
    
    bars = ax2.bar(range(len(states)), final_hazards, color=colors)
    ax2.set_xlabel('State')
    ax2.set_ylabel('Final Cumulative Hazard')
    ax2.set_title('Final Cumulative Hazard by State')
    ax2.set_xticks(range(len(states)))
    ax2.set_xticklabels(states, rotation=45, ha='right', fontsize=8)
    
    # Add value labels on bars
    for bar, value in zip(bars, final_hazards):
        ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01,
                f'{value:.3f}', ha='center', va='bottom', fontsize=8)
    
    plt.tight_layout()
    return fig

def analyze_transition_types(survival_data, state_definitions):
    """
    Analyze different types of transitions.
    """
    print("\nTransition Type Analysis:")
    print("-" * 40)
    
    # The survival_data already has from_solo, from_grupo, from_fermo, to_solo, to_grupo, to_fermo
    survival_with_states = survival_data.copy()
    
    # Calculate changes in each component
    survival_with_states['delta_solo'] = survival_with_states['to_solo'] - survival_with_states['from_solo']
    survival_with_states['delta_grupo'] = survival_with_states['to_grupo'] - survival_with_states['from_grupo']
    survival_with_states['delta_fermo'] = survival_with_states['to_fermo'] - survival_with_states['from_fermo']
    
    # Categorize transitions
    transition_types = []
    for _, row in survival_with_states.iterrows():
        ds, dg, df = row['delta_solo'], row['delta_grupo'], row['delta_fermo']
        if ds > 0 and dg < 0:
            transition_types.append('Grupo→Solo')
        elif ds < 0 and dg > 0:
            transition_types.append('Solo→Grupo')
        elif df > 0:
            transition_types.append('→Fermo')
        elif df < 0:
            transition_types.append('Fermo→')
        else:
            transition_types.append('Other')
    
    survival_with_states['transition_type'] = transition_types
    
    # Analyze by transition type
    type_summary = survival_with_states.groupby('transition_type').agg({
        'duration': ['count', 'mean', 'std'],
        'event': ['sum', 'mean']
    }).round(3)
    
    print(type_summary)
    
    return survival_with_states

def main():
    """
    Main analysis function.
    """
    print("Nelson-Aalen Analysis of SGF State Transitions")
    print("=" * 50)
    
    # Load data
    survival_data, state_definitions = load_and_analyze_sgf_data()
    if survival_data is None:
        return
    
    # Analyze by starting state
    results = analyze_by_starting_state(survival_data, state_definitions)
    
    # Create plots
    fig = plot_nelson_aalen_curves(results)
    fig.savefig('nelson_aalen_sgf_analysis.png', dpi=300, bbox_inches='tight')
    print(f"\nSaved plot: nelson_aalen_sgf_analysis.png")
    
    # Analyze transition types
    survival_with_states = analyze_transition_types(survival_data, state_definitions)
    
    # Create summary table
    summary_data = []
    for state_idx, result in results.items():
        summary_data.append({
            'State_Index': state_idx,
            'State_SGF': result['state_name'],
            'Observations': result['n_obs'],
            'Events': result['n_events'],
            'Event_Rate': result['n_events'] / result['n_obs'],
            'Final_Cumulative_Hazard': result['final_hazard']
        })
    
    summary_df = pd.DataFrame(summary_data)
    summary_df = summary_df.sort_values('Observations', ascending=False)
    summary_df.to_csv('nelson_aalen_summary.csv', index=False)
    print(f"Saved summary: nelson_aalen_summary.csv")
    
    plt.show()

if __name__ == "__main__":
    main()
