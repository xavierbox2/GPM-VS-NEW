#include <algorithm>

#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif

#include "VisageDeckSimulationOptions.h"

void VisageDeckSimulationOptions::configure_default_status()
{
    //unordered_map<std::string, std::string> hashedValues;
    //hashedValues.insert( std::pair<std::string, std::string>( "status_file", "0" ) );
    //set_command( "STATUS", "", &hashedValues );
}

void VisageDeckSimulationOptions::configure_default_header()
{
    unordered_map<std::string, std::string> hashedValues;
    hashedValues.insert(std::pair<std::string, std::string>("module", "static"));
    hashedValues.insert(std::pair<std::string, std::string>("solution", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("analysis", "3-D"));
    hashedValues.insert(std::pair<std::string, std::string>("sautoplastictime", "5"));
    hashedValues.insert(std::pair<std::string, std::string>("vp_timestep_factor", "0.5"));
    hashedValues.insert(std::pair<std::string, std::string>("sconvergencemethod", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sanisotropic", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Slocalanisotropy", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sjoint_anisotropy", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Smpc", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Nmpc", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sporepressures", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("numMeshBlocks", "1"));

    hashedValues.insert(std::pair<std::string, std::string>("Nelements", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Nnodes", "8"));
    hashedValues.insert(std::pair<std::string, std::string>("Nmaterials", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Nconstraints", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("gaussrulebricks", "2"));
    hashedValues.insert(std::pair<std::string, std::string>("Niterations", "1"));

    hashedValues.insert(std::pair<std::string, std::string>("Sdisplacements", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Ndisplacements", "0"));

    hashedValues.insert(std::pair<std::string, std::string>("Nsub_increments", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Nfaults", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sfaults", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Ndfn", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sdfn", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Nmax_ele_per_dfn", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Nmax_ele_per_fault", "0"));

    hashedValues.insert(std::pair<std::string, std::string>("vcreep_tolerance", "1.000000E+000"));

    hashedValues.insert(std::pair<std::string, std::string>("Sedgeload", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Stemperatures", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sgravity", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Saturation", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Sconnect", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Nconnect", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Npointload", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Screep", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Saccelerator", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("sperformance_type", "2"));

    hashedValues.insert( std::pair<std::string, std::string>( "nyield_gp_number", "1" ) );
    hashedValues.insert( std::pair<std::string, std::string>( "Nsub_increments", "10" ) );
    hashedValues.insert( std::pair<std::string, std::string>( "Nquickcalculation", "50" ) );
    hashedValues.insert( std::pair<std::string, std::string>( "Niterations", "10" ) );
    hashedValues.insert( std::pair<std::string, std::string>( "suse_all_disp", "1" ) );
    //hashedValues.insert( std::pair<std::string, std::string>( "status_file", "0" ) );

 

    //AsteriskKey *command = new AsteriskKey("HEADER", "", "*", &hashedValues);
    //commands.insert(std::pair<std::string, AsteriskKey*>("HEADER", command));
    set_command("HEADER", "", &hashedValues);
}

void VisageDeckSimulationOptions::configure_default_solver_section()
{
    unordered_map<std::string, std::string> hashedValues;

    hashedValues.insert(std::pair<std::string, std::string>("type", "7"));
    hashedValues.insert(std::pair<std::string, std::string>("sdeflation ", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("vtolerance", "1.00000E-08"));
    hashedValues.insert(std::pair<std::string, std::string>("niter_stagnation", "5"));
    hashedValues.insert(std::pair<std::string, std::string>("serrortrap", "3"));
    //hashes.Add("sgpu", "0");
    //hashes.Add("ngpu", "0");
    hashedValues.insert(std::pair<std::string, std::string>("device", ""));

    //AsteriskKey *command = new AsteriskKey("SOLVER", "", "*", &hashedValues);
    //commands.insert(std::pair<std::string, AsteriskKey*>("SOLVER", command));
    set_command("SOLVER", "", &hashedValues);
}

void VisageDeckSimulationOptions::configure_default_restart()
{
    set_cummulate_displacemnts(true);
    //set_shift_k(false);

    unordered_map<std::string, std::string> hashedValues;
    std::string current_step = std::to_string(step());
    std::string prev_step = std::to_string(max(0, step() - 1));

    hashedValues.insert(std::pair<std::string, std::string>("Swriterestart", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Nwrite_number", current_step));
    hashedValues.insert(std::pair<std::string, std::string>("Sreadrestart", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("Suse_hdf5", "0"));

    set_command("RESTART", "", &hashedValues);
}

void VisageDeckSimulationOptions::configure_multistep_restart()
{
    unordered_map<std::string, std::string> hashedValues;
    std::string current_step = std::to_string(step());
    std::string prev_step = std::to_string(max(0, step() - 1));

    hashedValues.insert(std::pair<std::string, std::string>("Swriterestart", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("Nwrite_number", current_step));
    hashedValues.insert(std::pair<std::string, std::string>("Sreadrestart", step() > 0 ? " 1 " : " 0 "));
    hashedValues.insert(std::pair<std::string, std::string>("Nread_number", step() > 0 ? prev_step : " 0 "));

    //if (get_cummulate_displacemnts())
    //    hashedValues.insert(std::pair<std::string, std::string>("Saccumulatedisp", step() > 0 ? "1" : " 0 "));

    hashedValues.insert(std::pair<std::string, std::string>("Suse_hdf5", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("writerestart_file", model_name()));

    if (step() <= 0)
    {
        delete_command("LOADSTEP");
    }
    else
    {
        set_command("LOADSTEP", "1");
    }

    if (step() >= 1)
    {
        //if (get_shift_k())
        //    hashedValues.insert(std::pair<std::string, std::string>("shift_k", "1"));

        hashedValues.insert(std::pair<std::string, std::string>("readrestart_file", model_name()));

        if (step() >= 2)
        {
            set_replace_instruction("RESULTS", "append_loadstep", std::to_string(step() - 1));

            hashedValues.insert(std::pair<std::string, std::string>("szerotime", "1"));
        }
    }

    set_command("RESTART", "", &hashedValues);
}

void VisageDeckSimulationOptions::configure_default_results()
{
    unordered_map<std::string, std::string> hashedValues;

    hashedValues.insert(std::pair<std::string, std::string>("petrel_units", "metric"));
    hashedValues.insert(std::pair<std::string, std::string>("material_data", "1"));


    hashedValues.insert(std::pair<std::string, std::string>("petrel", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("GID", "0"));


    hashedValues.insert(std::pair<std::string, std::string>("ele_pressures", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_strain", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_stresses", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_total_stresses", "1"));

    hashedValues.insert(std::pair<std::string, std::string>("ele_yield_values", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_failure_mode", "1"));

    hashedValues.insert(std::pair<std::string, std::string>("ele_fault_disps", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_fault_strain", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_fracture_disps", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_fracture_strain", "0"));

    hashedValues.insert(std::pair<std::string, std::string>("unify_faults", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("unify_fractures", "1"));

    hashedValues.insert(std::pair<std::string, std::string>("nodal_total_disps", "2"));
    hashedValues.insert(std::pair<std::string, std::string>("petrel_nodal", "2"));

    hashedValues.insert(std::pair<std::string, std::string>("ele_creep_strains", "0"));
    hashedValues.insert(std::pair<std::string, std::string>("ele_dvt_variation", "0"));
    hashedValues.insert( std::pair<std::string, std::string>( "ele_tot_pl_strain", "1" ) );
    hashedValues.insert(std::pair<std::string, std::string>("permeability_update", "0"));
    set_command("RESULTS", "", &hashedValues);
}

void VisageDeckSimulationOptions::configure_default_echo()
{
    unordered_map<std::string, std::string> hashedValues;
    hashedValues.insert(std::pair<std::string, std::string>("snodes ", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("scgm", "-100"));
    hashedValues.insert(std::pair<std::string, std::string>("selements", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("smaterials", "1"));
    hashedValues.insert(std::pair<std::string, std::string>("sload", "1"));
    set_command("NOECHO", "", &hashedValues);
}

void VisageDeckSimulationOptions::update_step()
{
    configure_multistep_restart();

    set_command("MODELNAME", _model_name);

    std::string year = std::to_string(1900 + step());
    std::string date = std::to_string(1/*step()*/) + " \n01/01/" + year + " 00:00:00.000";
    set_command("PRINTDATES", date);

    //update grid nodes..number of materials, etc if needed.
    Structured_Geometry_Size _geometry_size = geometry_size();
    int _cells[3] = { _geometry_size.num_nodes_i - 1, _geometry_size.num_nodes_j - 1,_geometry_size.num_nodes_k - 1 };
    int nCells = _cells[0] * _cells[1] * _cells[2];
    int nNodes = (1 + _cells[0]) * (1 + _cells[1]) * (1 + _cells[2]);

    set_replace_instruction("HEADER", "Nelements", std::to_string(nCells));
    set_replace_instruction("HEADER", "Nnodes", std::to_string(nNodes));
    //assuming one material per cell. Even if it is repeated.
    set_replace_instruction("HEADER", "Nmaterials", std::to_string(nCells));

    //grid size, assuming it could change in-between steps.
    set_replace_instruction("STRUCTUREDGRID", "idimension", std::to_string(_cells[0]));
    set_replace_instruction("STRUCTUREDGRID", "jdimension", std::to_string(_cells[1]));
    set_replace_instruction("STRUCTUREDGRID", "kdimension", std::to_string(_cells[2]));
    set_replace_instruction("STRUCTUREDGRID", "ordering", "gpp");
}