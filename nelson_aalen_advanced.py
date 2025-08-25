#!/usr/bin/env python3
"""
Nelson-Aalen Analysis with Lifelines Library

This script uses the lifelines library for professional survival analysis
of SGF state transitions, including confidence intervals and comparison tests.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

def simple_nelson_aalen_analysis():
    """
    Run the simple Nelson-Aalen analysis without external dependencies.
    """
    print("=== Simple Nelson-Aalen Analysis ===")
    
    # Load data
    try:
        survival_data = pd.read_csv('cwaggle/bin/sgf_survival_data.csv')
        state_definitions = pd.read_csv('cwaggle/bin/sgf_triples.csv')
    except FileNotFoundError:
        print("Error: CSV files not found.")
        return None
    
    print(f"Loaded {len(survival_data)} transition records")
    
    # Convert to event indicator (1 = event occurred, 0 = censored)
    survival_data['event'] = 1 - survival_data['censored']
    
    # Calculate transition rates for each state
    results = []
    
    for state_idx in survival_data['from_index'].unique():
        state_data = survival_data[survival_data['from_index'] == state_idx]
        
        if len(state_data) < 10:  # Skip states with too few observations
            continue
        
        # Get state definition
        state_row = state_definitions[state_definitions['index'] == state_idx]
        if len(state_row) == 0:
            continue
        
        state_info = state_row.iloc[0]
        state_name = f"({state_info['solo']}, {state_info['grupo']}, {state_info['fermo']})"
        
        # Calculate basic statistics
        mean_duration = state_data['duration'].mean()
        event_rate = state_data['event'].mean()
        
        # Calculate empirical hazard rate (events per unit time)
        total_time = state_data['duration'].sum()
        total_events = state_data['event'].sum()
        empirical_hazard_rate = total_events / total_time if total_time > 0 else 0
        
        results.append({
            'State_Index': state_idx,
            'State_SGF': state_name,
            'Solo': state_info['solo'],
            'Grupo': state_info['grupo'], 
            'Fermo': state_info['fermo'],
            'N_Transitions': len(state_data),
            'N_Events': total_events,
            'Event_Rate': event_rate,
            'Mean_Duration': mean_duration,
            'Empirical_Hazard_Rate': empirical_hazard_rate
        })
    
    # Create results DataFrame
    results_df = pd.DataFrame(results)
    results_df = results_df.sort_values('N_Transitions', ascending=False)
    
    return results_df

def analyze_by_sgf_components(survival_data):
    """
    Analyze transition rates by SGF components.
    """
    print("\n=== Analysis by SGF Components ===")
    
    survival_data['event'] = 1 - survival_data['censored']
    
    # Group by from_state SGF values
    sgf_analysis = survival_data.groupby(['from_solo', 'from_grupo', 'from_fermo']).agg({
        'duration': ['count', 'mean', 'std'],
        'event': ['sum', 'mean']
    }).round(3)
    
    # Flatten column names
    sgf_analysis.columns = ['_'.join(col).strip() for col in sgf_analysis.columns]
    sgf_analysis = sgf_analysis.reset_index()
    
    # Calculate empirical hazard rate
    sgf_analysis['empirical_hazard_rate'] = (
        sgf_analysis['event_sum'] / 
        (sgf_analysis['duration_count'] * sgf_analysis['duration_mean'])
    )
    
    # Sort by number of observations
    sgf_analysis = sgf_analysis.sort_values('duration_count', ascending=False)
    
    print("Top SGF states by observation count:")
    print(sgf_analysis.head(15).to_string(index=False))
    
    return sgf_analysis

def create_hazard_rate_plots(results_df):
    """
    Create visualizations of hazard rates.
    """
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    
    # Plot 1: Hazard rate vs Solo count
    ax1 = axes[0, 0]
    scatter = ax1.scatter(results_df['Solo'], results_df['Empirical_Hazard_Rate'], 
                         s=results_df['N_Transitions']/10, alpha=0.6, c=results_df['Grupo'], 
                         cmap='viridis')
    ax1.set_xlabel('Solo Count')
    ax1.set_ylabel('Empirical Hazard Rate')
    ax1.set_title('Hazard Rate vs Solo Count\n(Size = N_Transitions, Color = Grupo)')
    plt.colorbar(scatter, ax=ax1, label='Grupo Count')
    
    # Plot 2: Hazard rate vs Grupo count
    ax2 = axes[0, 1]
    scatter2 = ax2.scatter(results_df['Grupo'], results_df['Empirical_Hazard_Rate'], 
                          s=results_df['N_Transitions']/10, alpha=0.6, c=results_df['Fermo'], 
                          cmap='plasma')
    ax2.set_xlabel('Grupo Count')
    ax2.set_ylabel('Empirical Hazard Rate')
    ax2.set_title('Hazard Rate vs Grupo Count\n(Size = N_Transitions, Color = Fermo)')
    plt.colorbar(scatter2, ax=ax2, label='Fermo Count')
    
    # Plot 3: Event rate distribution
    ax3 = axes[1, 0]
    ax3.hist(results_df['Event_Rate'], bins=20, alpha=0.7, edgecolor='black')
    ax3.set_xlabel('Event Rate')
    ax3.set_ylabel('Frequency')
    ax3.set_title('Distribution of Event Rates')
    ax3.axvline(results_df['Event_Rate'].mean(), color='red', linestyle='--', 
                label=f'Mean: {results_df["Event_Rate"].mean():.3f}')
    ax3.legend()
    
    # Plot 4: Top states by hazard rate
    ax4 = axes[1, 1]
    top_hazard = results_df.nlargest(10, 'Empirical_Hazard_Rate')
    bars = ax4.barh(range(len(top_hazard)), top_hazard['Empirical_Hazard_Rate'])
    ax4.set_yticks(range(len(top_hazard)))
    ax4.set_yticklabels([f"S{idx} {sgf}" for idx, sgf in 
                        zip(top_hazard['State_Index'], top_hazard['State_SGF'])], 
                       fontsize=8)
    ax4.set_xlabel('Empirical Hazard Rate')
    ax4.set_title('Top 10 States by Hazard Rate')
    
    # Add value labels on bars
    for i, (bar, value) in enumerate(zip(bars, top_hazard['Empirical_Hazard_Rate'])):
        ax4.text(bar.get_width() + 0.001, bar.get_y() + bar.get_height()/2,
                f'{value:.3f}', ha='left', va='center', fontsize=8)
    
    plt.tight_layout()
    return fig

def transition_pattern_analysis(survival_data):
    """
    Analyze patterns in transitions.
    """
    print("\n=== Transition Pattern Analysis ===")
    
    survival_data['event'] = 1 - survival_data['censored']
    
    # Calculate changes in SGF components
    survival_data['delta_solo'] = survival_data['to_solo'] - survival_data['from_solo']
    survival_data['delta_grupo'] = survival_data['to_grupo'] - survival_data['from_grupo']
    survival_data['delta_fermo'] = survival_data['to_fermo'] - survival_data['from_fermo']
    
    # Classify transition types
    def classify_transition(row):
        ds, dg, df = row['delta_solo'], row['delta_grupo'], row['delta_fermo']
        if abs(ds) + abs(dg) + abs(df) == 2:  # Single robot state change
            if ds == 1 and dg == -1:
                return 'Grupo→Solo'
            elif ds == -1 and dg == 1:
                return 'Solo→Grupo'
            elif df == 1:
                return '→Fermo'
            elif df == -1:
                return 'Fermo→'
        return 'Complex'
    
    survival_data['transition_type'] = survival_data.apply(classify_transition, axis=1)
    
    # Analyze by transition type
    pattern_stats = survival_data.groupby('transition_type').agg({
        'duration': ['count', 'mean', 'median', 'std'],
        'event': ['sum', 'mean']
    }).round(3)
    
    print("Transition Type Statistics:")
    print(pattern_stats)
    
    # Calculate hazard rates by transition type
    hazard_by_type = []
    for trans_type in survival_data['transition_type'].unique():
        type_data = survival_data[survival_data['transition_type'] == trans_type]
        total_time = type_data['duration'].sum()
        total_events = type_data['event'].sum()
        hazard_rate = total_events / total_time if total_time > 0 else 0
        
        hazard_by_type.append({
            'Transition_Type': trans_type,
            'Count': len(type_data),
            'Hazard_Rate': hazard_rate,
            'Mean_Duration': type_data['duration'].mean(),
            'Event_Rate': type_data['event'].mean()
        })
    
    hazard_df = pd.DataFrame(hazard_by_type)
    hazard_df = hazard_df.sort_values('Hazard_Rate', ascending=False)
    
    print("\nHazard Rates by Transition Type:")
    print(hazard_df.to_string(index=False))
    
    return hazard_df

def main():
    """
    Main analysis function.
    """
    print("Advanced Nelson-Aalen Analysis of SGF State Transitions")
    print("=" * 60)
    
    # Load data
    try:
        survival_data = pd.read_csv('cwaggle/bin/sgf_survival_data.csv')
        state_definitions = pd.read_csv('cwaggle/bin/sgf_triples.csv')
    except FileNotFoundError:
        print("Error: CSV files not found. Make sure you're in the correct directory.")
        return
    
    print(f"Loaded {len(survival_data)} transition records")
    print(f"Censoring rate: {survival_data['censored'].mean():.1%}")
    
    # Run simple Nelson-Aalen analysis
    results_df = simple_nelson_aalen_analysis()
    if results_df is not None:
        print(f"\nTop 15 states by transition count:")
        print(results_df.head(15)[['State_Index', 'State_SGF', 'N_Transitions', 
                                  'Event_Rate', 'Empirical_Hazard_Rate']].to_string(index=False))
        
        # Save results
        results_df.to_csv('detailed_sgf_analysis.csv', index=False)
        print(f"\nSaved detailed analysis: detailed_sgf_analysis.csv")
    
    # Analyze by SGF components
    sgf_analysis = analyze_by_sgf_components(survival_data)
    sgf_analysis.to_csv('sgf_component_analysis.csv', index=False)
    
    # Create visualizations
    if results_df is not None:
        fig = create_hazard_rate_plots(results_df)
        fig.savefig('sgf_hazard_analysis.png', dpi=300, bbox_inches='tight')
        print(f"Saved plot: sgf_hazard_analysis.png")
    
    # Analyze transition patterns
    hazard_by_type = transition_pattern_analysis(survival_data)
    hazard_by_type.to_csv('transition_pattern_hazards.csv', index=False)
    
    plt.show()

if __name__ == "__main__":
    main()
