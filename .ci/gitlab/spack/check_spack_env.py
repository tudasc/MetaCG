#!/usr/bin/env spack python
"""
Check if the current Spack environment is fully installed.

Usage:
    spack python check_spack_env.py
"""

import sys
import spack.environment as ev
import spack.store


def check_environment_installed():
    """
    Check if all specs in the active Spack environment are installed.
    
    Returns:
        int: Exit code (0 = success, 1 = incomplete installation)
    """
    # Get the active environment
    env = ev.active_environment()
    
    if env is None:
        print("Error: No active Spack environment found.")
        print("Please activate an environment with 'spack env activate <env_name>'")
        sys.exit(2)
    
    print(f"Checking environment: {env.name}")
    print(f"Environment path: {env.path}")
    print("-" * 60)
    
    # Get all specs in the environment
    all_specs = env.all_specs()
    root_specs = env.user_specs
    
    if not all_specs:
        print("Environment is empty (no specs defined).")
        return 0
    
    total_specs = len(all_specs)
    installed_specs = []
    external_specs = []
    missing_specs = []
    missing_root_specs = []
    missing_dep_specs = []
    
    # Create set of root spec names for quick lookup
    root_spec_names = {spec.name for spec in root_specs}
    
    # Check installation status of each spec
    for spec in all_specs:
        # Check if the spec is installed
        if spec.installed:
            installed_specs.append(spec)
            # Check if this is an external package
            # External packages can be detected via spec.external or spec.external_path
            if spec.external or spec.external_path:
                external_specs.append(spec)
        else:
            missing_specs.append(spec)
            # Check if this is a root spec or a dependency
            if spec.name in root_spec_names:
                missing_root_specs.append(spec)
            else:
                missing_dep_specs.append(spec)
    
    print()
    print(f"Total specs: {total_specs}")
    print(f"Installed: {len(installed_specs)}")
    print(f"Missing: {len(missing_specs)}")
    
    is_fully_installed = len(missing_specs) == 0
    
    if is_fully_installed:
        print("\n✓ Environment is fully installed!")
        return 0
    else:
        print("\n✗ Environment is NOT fully installed.")
        
        if missing_root_specs:
            print(f"\nMissing root package(s): {len(missing_root_specs)}")
            for spec in missing_root_specs:
                print(f"  - {spec}")
        
        if missing_dep_specs:
            print(f"\nMissing dependencies: {len(missing_dep_specs)}")
            for spec in missing_dep_specs:
                print(f"  - {spec}")
        
        print("\nTo install missing packages, run:")
        print("  spack install")
        return 1
    
    return is_fully_installed, details


def main():
    """Main entry point."""
    try:
        exit_code = check_environment_installed()
        sys.exit(exit_code)
            
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)


if __name__ == "__main__":
    main()
