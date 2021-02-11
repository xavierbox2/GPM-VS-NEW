#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif

#include <array>
#include <algorithm>
#include <limits>
#include <numeric>
#include <filesystem>
#include <optional>
#include <algorithm>

#include "Definitions.h"
#include "Range.h"
#include "Vector3.h"
#include "ArrayData.h"
#include "VisageDeckWritter.h"


std::optional< pair<int, std::string> >VisageDeckWritter::write_dvt_tables( VisageDeckSimulationOptions* options, ArrayData* arrays )
{
    if((arrays == nullptr) || (!arrays->contains( "dvt_table_index" ) || (!options->use_tables( )) || (options->_tables.size( ) <= 0)))
    {
        options->set_replace_instruction( "RESULTS", "ele_dvt_variation", "0" );
        options->set_replace_instruction( "HEADER", "ndvttables", "0" );
        return optional<  pair<int, std::string> >{};
    }

    std::string file_base = get_filename( options->step( ), options->model_name( ), "dvt" );
    std::string file_name = options->path( ) + "\\" + file_base;
    ofstream file( file_name );

    bool write_header = true;

    //table with the highest resolution 
    auto it = std::max_element( options->_tables.begin( ), options->_tables.end( ), []( const auto& p1, const auto& p2 ) { return p1.second.size( ) < p2.second.size( ); } );
    int table_size = it->second.size( );

    for(auto& p : options->_tables)
    {
        Table& t = p.second;

        if(write_header)
        {
            file << "*DVTTABLES" << std::endl << "1" << " " << t.size( ) << endl << endl << t.controller_var_name( ) << " " << t.dependant_name( ) << endl;
            write_header = false;
        }

        t.resample( table_size );

        for(const auto& pair : t.value_pairs( ))
            file << pair.first << " " << pair.second << endl;

        file << endl;
    }
    file.close( );

    options->set_replace_instruction( "RESULTS", "ele_dvt_variation", "1" );
    options->set_replace_instruction( "HEADER", "ndvttables", std::to_string( options->_tables.size( ) ) );//std::to_string(options->_tables.size()));

    return optional<  pair<int, std::string> > { make_pair( options->_tables.size( ), file_base ) };
}

std::string VisageDeckWritter::write_mii_files( VisageDeckSimulationOptions* options, vector<std::string>& files ) //requires required path model names options and geometry dfescripotion (geometry size)
{
    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "MII" );

    if(ofstream file( file_name ); file)
    {
        //the order matters so we write these first 
        auto echo = options->get_command( "NOECHO" );
        auto header = options->get_command( "HEADER" );
        auto name = options->get_command( "MODELNAME" );
        std::string s = "";

        std::string first_commands[] = { "MODELNAME","NOECHO",  "HEADER" };
        for(std::string command_name : first_commands)
        {
            if(auto command = options->get_command( command_name ); command != nullptr)
            {
                s += command->to_string( ) + "\n";
            }
        }

        std::vector< InstructionBlock* > commands = options->get_commands( );
        for(int n = 0; n < commands.size( ); n++)
        {
            const std::string command_name = commands[n]->name;
            if(find( begin( first_commands ), end( first_commands ), command_name ) == end( first_commands ))
            {
                s += commands[n]->to_string( ) + "\n";
            }
        }

        for(auto file : files)
        {
            s += "INCLUDE " + file + "\n";
        }

        s += "*END\n";
        file << get_header( ) << std::endl;
        file << s << std::endl;
        file.close( );

        return file_name;
    }

    return "";

}

std::string VisageDeckWritter::write_plastic_mat_file( VisageDeckSimulationOptions* options, ArrayData* data, const map<std::string, float>* unit_conversion )
{
    function<float( std::string, const map<std::string, float>* )> units = []( std::string s, const map<std::string, float>* unit )
    { return unit != nullptr && unit->find( s ) != unit->end( ) ? unit->at( s ) : 1.0f;
    };

    function<void( ofstream&, int, std::string, const vector<float>&, float )> write_property_array = []( ofstream& file, int grid_size, std::string vs_keyword, const vector<float>& phi, float scale )
    {
        file << vs_keyword << std::endl;

        if(phi.size( ) == 1)
        {
            file << grid_size << " [   " << phi[0] * scale << " ] " << endl;
        }

        else
        {
            for(int n = 0; n < grid_size; n++) file << " " << phi[n] * scale << endl;
        }

        file << std::endl;
    };

    function<bool( ofstream&, int, ArrayData*, const map<std::string, float>* )> write_elastic =
        [units, write_property_array]( ofstream& file, int grid_size, ArrayData* data, const map<std::string, float>* units_convert )-> bool
    {

        if(!data->contains( "YOUNGMOD" ))return false;
        if(!data->contains( "POISSONR" ))return false;
        if(!data->contains( "DENSITY" ))return false;

        const vector<float>& ym = data-> get_array( "YOUNGMOD" );
        const vector<float>& pr = data->get_array( "POISSONR" );
        file << "*ELASTIC_DATA,NOCOM" << std::endl;

        float scale1 = units( "YOUNGMOD", units_convert ), scale2 = units( "POISSONR", units_convert );
        size_t mult_ym = (ym.size( ) > 1) ? 1 : 0;
        size_t mult_pr = (pr.size( ) > 1) ? 1 : 0;
        for(size_t n = 0; n < grid_size; n++) file << " 1  " << ym[n * mult_ym] * scale1 << " " << pr[n * mult_pr] * scale2 << endl;
        file << endl;

        vector<float>& poro = data->get_or_create_array( "POROSITY", 0.3f, 1 );
        write_property_array( file, grid_size, "*POROSITY, NOCOM", poro, 1.0 );

        vector<float>& biot = data->get_or_create_array( "BIOTC", 1.0, 1 );//.data();
        write_property_array( file, grid_size, "*BIOTS_MODULUS,NOCOM", biot, 1.0f );

        vector<float>& rho = data->get_array( "DENSITY" );
        float scale3 = units( "DENSITY", units_convert );
        write_property_array( file, grid_size, "*SOLID_UNIT_W,NOCOM", rho, scale3 );

        return true;
    };

    function<bool( ofstream&, int, ArrayData*, const map<std::string, float>* )> write_mohr_coulomb =
        [units, write_property_array]( ofstream& file, int grid_size, ArrayData* data, const map<std::string, float>* units_convert )-> bool
    {
        if(!data->contains( "COHESION" ))return false;
        if(!data->contains( "TENSILE_STRENGTH" ))return false;

        vector<float>& cohesion = data->get_array( "COHESION" );     int c = cohesion.size( ) > 1 ? 1 : 0;
        vector<float>& tensile = data->get_array( "TENSILE_STRENGTH" ); int t = tensile.size( ) > 1 ? 1 : 0;
        vector<float>& friction = data->get_or_create_array( "FRICTION", 30.0f, 1 ); int f = friction.size( ) > 1 ? 1 : 0;
        vector<float>& dilation = data->get_or_create_array( "DILATION", 15.0f, 1 ); int d = dilation.size( ) > 1 ? 1 : 0;
        vector<float>& fluidity = data->get_or_create_array( "FLUIDITY", 1.0, 1 ); int fl = fluidity.size( ) > 1 ? 1 : 0;
        vector<float>& hardening = data->get_or_create_array( "HARDENING", -0.005, 1 ); int h = hardening.size( ) > 1 ? 1 : 0;
        vector<float>& coh_change = data->get_or_create_array( "RESIDUALCOHESION", 1.0f, 1 ); int ch = coh_change.size( ) > 1 ? 1 : 0;

        float coh_scale_factor = units( "COHESION", units_convert ), tens_scale_factor = units( "TENSILE_STRENGTH", units_convert );

        file << "*YIELD_DATA,NOCOM" << std::endl;
        for(int n = 0; n < grid_size; n++)
        {
            file << -1 << " " << coh_scale_factor * cohesion[n * c] << " " << friction[n * f] << " " << dilation[n * d] << " " << fluidity[n * fl] << " " << hardening[n * h] << " " << tens_scale_factor * tensile[n * t] << " " << coh_change[n * ch] * coh_scale_factor << endl;//coh_scale_factor * coh_change[n] << endl;
        }

        return true;
    };

    function<bool( ofstream&, int, ArrayData*, const map<std::string, float>* )> write_caprock =
        [units, write_property_array]( ofstream& file, int grid_size, ArrayData* data, const map<std::string, float>* units_convert )-> bool
    {
        if(!data->contains( "COHESION" ))return false;
        if(!data->contains( "TENSILE_STRENGTH" ))return false;
        if(!data->contains( "PORE_COLLAPSE" ))return false;
        if(!data->contains( "POROSITY" ))return false;

        vector<float>& pore_collapse = data->get_array( "PORE_COLLAPSE" );
        vector<float>& friction = data->get_or_create_array( "FRICTION", 30.0f, 1 );
        vector<float>& hardening = data->get_or_create_array( "HARDENING", -0.005, 1 );
        vector<float>& fluidity = data->get_or_create_array( "FLUIDITY", 1.0, 1 );
        vector<float>& transition_factor = data->get_or_create_array( "TRANSITION_FACTOR", 0.1f, 1 );
        vector<float>& tensile = data->get_array( "TENSILE_STRENGTH" );
        vector<float>& porosity = data->get_array( "POROSITY" );
        vector<float>& ellipse_factor = data->get_or_create_array( "RADIUS_ELLIPSE_FACTOR", 0.5f, 1 );
        vector<float>& dilation = data->get_or_create_array( "DILATION", 15.0f, 1 );
        vector<float>& deviatoric = data->get_or_create_array( "DEVIATORIC", 1.0f, 1 );
        vector<float>& cohesion = data->get_array( "COHESION" );

        int pc = pore_collapse.size( ) > 1 ? 1 : 0;
        int fr = friction.size( ) > 1 ? 1 : 0;
        int ha = hardening.size( ) > 1 ? 1 : 0;
        int fl = fluidity.size( ) > 1 ? 1 : 0;
        int por = porosity.size( ) > 1 ? 1 : 0;
        int te = tensile.size( ) > 1 ? 1 : 0;
        int ell = ellipse_factor.size( ) > 1 ? 1 : 0;
        int dil = dilation.size( ) > 1 ? 1 : 0;
        int dev = deviatoric.size( ) > 1 ? 1 : 0;
        int c = cohesion.size( ) > 1 ? 1 : 0;

        float coh_scale_factor = units( "COHESION", units_convert );
        float tens_scale_factor = units( "TENSILE_STRENGTH", units_convert );
        float pc_scale_factor = units( "PORE_COLLAPSE", units_convert );

        //transition is a presure unit, factor times cohesion. Lets compute it here 
        vector<float> transition( std::max( cohesion.size( ), transition_factor.size( ) ) );
        int tf = transition.size( ) > 1 ? 1 : 0;
        for(int n = 0; n < transition.size( ); n++)
            transition[n] = coh_scale_factor * cohesion[c * n] * transition_factor[tf * n];


        //there is a Qo parameter that depends on cohesion 
        float pi = acos( -1.0 ) / 180.0;
        vector<float> qo( std::max( cohesion.size( ), friction.size( ) ) );
        for(int n = 0; n < qo.size( ); n++)
        {
            float phi = friction[n * fr] * pi;
            qo[n] = coh_scale_factor * cohesion[c * n] * 6.0 * cos( phi ) / (3.0 - sin( phi ));
        }
        int q = qo.size( ) > 1 ? 1 : 0;

        file << "*YIELD_DATA,NOCOM" << std::endl;
        for(int n = 0; n < grid_size; n++)
        {
            file << 7 << " "
                << pc_scale_factor * pore_collapse[n * pc] << " "
                << friction[n * fr] << " "
                << hardening[n * ha] << " "
                << fluidity[n * fl] << " "
                << transition[n * tf] << " "
                << tens_scale_factor * tensile[n * te] << " "
                << porosity[n * por] << " "
                << ellipse_factor[n * ell] << " "
                << dilation[n * dil] << " "
                << deviatoric[n * dev] << " "
                << qo[n * q] << std::endl;
        }

        return true;
    };

    std::string mat_file = get_filename( options->step( ), options->model_name( ), "mat" );
    std::string file_name = options->path( ) + "\\" + mat_file;

    if(ofstream file( file_name ); file)
    {
        int grid_size = options->geometry( )->num_cells( );

        if(!write_elastic( file, grid_size, data, unit_conversion )) return "";
        if(data->id( ) == WellKnownVisageNames::FailureMode::ELASTIC) return  mat_file;

        if(data->id( ) == WellKnownVisageNames::FailureMode::MOHRCOULOMB)
            return write_mohr_coulomb( file, grid_size, data, unit_conversion ) ? mat_file : "";

        if(data->id( ) == WellKnownVisageNames::FailureMode::CAPROCK)
            return write_caprock( file, grid_size, data, unit_conversion ) ? mat_file : "";

        file.close( );
    }

    return "";
}

std::string VisageDeckWritter::write_mat_files( VisageDeckSimulationOptions* options, ArrayData* arrays, const map<std::string, float>* unit_conversion ) //requires required path model names options and geometry dfescripotion (geometry size)
{
    if(arrays == NULL) return false;

    return write_plastic_mat_file( options, arrays, unit_conversion );
}

//we assume that the elements are top-down.
pair<int, std::string> VisageDeckWritter::write_edge_loads( VisageDeckSimulationOptions* options, std::vector<float>* xyz )
{
    /*
 *EDGELOADS
16
         1          4
         6          7          2          1
0.00000E+000   0.00000E+000   0.00000E+000   0.00000E+000
0.00000E+000   0.00000E+000   0.00000E+000   0.00000E+000
1.32330E+004   1.32330E+004   1.32330E+004   1.32330E+004
         2          4
         7          8          3          2
0.00000E+000   0.00000E+000   0.00000E+000   0.00000E+000
0.00000E+000   0.00000E+000   0.00000E+000   0.00000E+000
1.32330E+004   1.32330E+004   1.32330E+004   1.32330E+004
(...)
 */

    std::string file_name = get_filename( options->step( ), options->model_name( ), "edg" );
    std::string file_path = options->path( ) + "\\" + file_name;
    ofstream file( file_path );
    if(!file) return pair<int, std::string>( 0, file_path );

    if((xyz == NULL) || (options->sea_level( ) == -999.0)) return pair<int, std::string>( 0, file_name );

    int total_loads = 0;
    vector<int> indices;
    vector<float> loads;
    total_loads = get_edge_loads( options, xyz, indices, loads );

    if(total_loads < 1)
    {
        file.close( );
        return pair<int, std::string>( 0, file_name );
    }

    file << get_header( ) << std::endl;
    file << "*EDGELOADS" << std::endl;
    file << total_loads << std::endl;

    for(int n = 0; n < total_loads; n++)
    {
        file << indices[5 * n] << '\t' << "4" << std::endl;
        for(int d : { 1, 2, 3, 4}) file << indices[5 * n + d] << '\t';
        file << std::endl;

        file << "0.0    0.0     0.0    0.0" << std::endl;
        file << "0.0    0.0     0.0    0.0" << std::endl;

        for(int d : { 0, 1, 2, 3}) file << loads[4 * n + d] << '\t';
        file << std::endl;
    }

    file.close( );

    return pair<int, std::string>( total_loads, file_name );
}

//compute the sea weight on the top layer
int VisageDeckWritter::get_edge_loads( VisageDeckSimulationOptions* options, std::vector<float>* xyz, vector<int>& indices, vector<float>& values )
{
    int total_loads = 0;
    float kpa_factor = 0.001f;

    auto geometry = options->geometry_size( );
    int nodes[3] = { geometry->num_nodes_i,geometry->num_nodes_j, geometry->num_nodes_k };
    int cells[3] = { geometry->num_nodes_i - 1,geometry->num_nodes_j - 1, geometry->num_nodes_k - 1 };

    float sea_level = options->sea_level( ), density = options->sea_water_density( ) * kpa_factor;
    for(int element = 0; element < cells[0] * cells[1]; element++)
    {
        int c_ij = element, cell_j = int( c_ij / (cells[0]) ), cell_i = c_ij - cell_j * cells[0];
        int n1 = cell_j * nodes[0] + cell_i, n2 = n1 + 1, n3 = n2 + nodes[0], n4 = n1 + nodes[0];

        array<int, 4> ns = { n1, n2, n3, n4 };
        array<float, 4> loads = { 0.0,0.0,0.0,0.0 };
        //lets average the 4 z coordinates of the nodes. If the average is below sea-level add the edge load.
        //if two are, add half the edgeload, if one is..1/4 of the edge load....

        int total = 0;
        for(int node_number = 0; node_number < 4; node_number++)
        {
            int node = ns[node_number];
            float depth = xyz->at( 3 * node + 1 ); //this is veriticazl in visage z-> -y
            if(depth > sea_level)
            {
                total += 1;
                loads[node_number] = -0.25f * (sea_level - depth) * density;
            }
        }

        if(total > 0)
        {
            indices.push_back( element + 1 );
            std::for_each( begin( ns ), end( ns ), [&indices]( int v ) {indices.push_back( 1 + v ); } );
            std::for_each( begin( loads ), end( loads ), [&values]( float v ) {values.push_back( v ); } );

            //for_each(begin(ns), end(ns), back_inserter(indices)[](int index) { });
            //for (int ii = 0; ii < 4; ii++)
            //{
            //	indices.push_back(ns[ii] + 1);
            //	values.push_back(loads[ii]);
            //}
            total_loads += 1;
        }
    }

    return total_loads;
}



std::string VisageDeckWritter::write_connections_files2( const map<int, int>& node_connections, VisageDeckSimulationOptions* options )
{
    std::map<int, std::vector<int>> connections_path;
    std::string file_base = get_filename( options->step( ), options->model_name( ), "rcn" );
    std::string file_name = (filesystem::path( options->path( ) ) /= filesystem::path( file_base )).string( );// + "\\" + file_base;


    if(node_connections.size( ) > 0)
    {
        std::vector<int> linked_connections( options->geometry( )->total_nodes( ) );
        std::iota( linked_connections.begin( ), linked_connections.end( ), 0 );

        for(const auto& pair : node_connections)
        {
            int i1 = pair.first, j1 = i1;
            int i2 = pair.second; //second, follows first, which follows j1 

            while(j1 != linked_connections[i1]) j1 = linked_connections[j1];

            linked_connections[i2] = j1;
            connections_path[j1].push_back( i2 );
        }


        ofstream file( file_name );
        file << "*CONNECT" << std::endl << std::endl;

        for(auto& pair : connections_path)
        {
            //int node1 [node2, node3, node4,.....]= 
            int node1 = pair.first;
            auto& v = pair.second;
            std::string s = std::to_string( 1 + node1 ) + " ";

            int max_chars = 240;
            int chars = s.size( );

            file << " " << 1 + v.size( ) << " 1110000000" << endl;
            for(auto n : v)
            {
                s += std::to_string( 1 + n ) + " ";
                if(s.size( ) > max_chars) { max_chars += 240; s += '\n'; }
            }


            file << s << endl;
        }

        file.close( );
    }

    options->set_replace_instruction( "HEADER", "Sconnect", (connections_path.size( ) > 0 ? " 1 " : " 0 ") );
    options->set_replace_instruction( "HEADER", "Nconnect", std::to_string( connections_path.size( ) ) );

    return connections_path.size( ) > 0 ? file_base : "";
}

std::string  VisageDeckWritter::write_connections_files( std::map<int, std::vector<int>>& connections, VisageDeckSimulationOptions* options )
{
    std::string file_base = get_filename( options->step( ), options->model_name( ), "rcn" );
    std::string file_name = options->path( ) + "\\" + file_base;
    ofstream file( file_name );
    file << "*CONNECT" << std::endl << std::endl;

    for(auto& pair : connections)
    {
        //int node1 [node2, node3, node4,.....]= 
        int node1 = pair.first;
        auto& v = pair.second;
        std::string s = std::to_string( 1 + node1 ) + " ";

        int max_chars = 240;
        int chars = s.size( );

        file << " " << 1 + v.size( ) << " 1110000000" << endl;
        for(auto n : v)
        {
            s += std::to_string( 1 + n ) + " ";
            if(s.size( ) > max_chars) { max_chars += 240; s += '\n'; }
        }


        file << s << endl;
    }

    file.close( );

    options->set_replace_instruction( "HEADER", "Sconnect", (connections.size( ) > 0 ? " 1 " : " 0 ") );
    options->set_replace_instruction( "HEADER", "Nconnect", std::to_string( connections.size( ) ) );

    return file_base;
}

//std::string  VisageDeckWritter::write_connections_files( map<int, int>& connections, VisageDeckSimulationOptions* options )
//{
//   std::string file_base = get_filename( options->step( ), options->model_name( ), "rcn" );
//   std::string file_name = options->path( ) + "\\" + file_base;
//    ofstream file( file_name );
//    file << "*CONNECT" << std::endl << std::endl;
//    for(pair<const int, int>& p : connections)
//    {
//        //int node1 = n1 + node_offset;
//        file << "		 2 1110000000" << endl;
//        file << 1 + p.first << "        " << 1 + p.second << endl;
//    }
//
//    file.close( );
//
//    options->set_replace_instruction( "HEADER", "Sconnect", (connections.size( ) > 0 ? " 1 " : " 0 ") );
//    options->set_replace_instruction( "HEADER", "Nconnect", std::to_string( connections.size( ) ) );
//
//    return file_base;
//}

//requires the explicit node coordinates in the visage reference frame
std::string VisageDeckWritter::write_node_files( VisageDeckSimulationOptions* options, std::vector<float>* xyz )//, bool &is_clock_wise)
{
    if(xyz == NULL) return "";

    std::string file_base = get_filename( options->step( ), options->model_name( ), "nod" );
    std::string file_name = options->path( ) + "\\" + file_base;
    ofstream file( file_name );
    if(!file)
        return "";
    else
    {
        file << get_header( ) << std::endl;
        file << "*COORDS, NOCOM" << std::endl;
        int node = 1;
        for(int n = 0; n < xyz->size( ); n += 3)
            file << node++ << '\t' << std::setprecision( 16 ) << xyz->at( n ) << '\t' << std::setprecision( 16 ) << xyz->at( n + 1 ) << '\t' << std::setprecision( 16 ) << xyz->at( n + 2 ) << std::endl;

        file.close( );
        /*     *COORDS, NOCOM
                1     0.0000000E+000     0.0000000E+000     0.0000000E+000
                2     5.0000000E+002     0.0000000E+000     0.0000000E+000
                3     0.0000000E+000     0.0000000E+000     7.5000000E+002
                4     5.0000000E+002     0.0000000E+000     7.5000000E+002
                5     0.0000000E+000 - 1.0000000E+003     0.0000000E+000
                6     5.0000000E+002 - 1.0000000E+003     0.0000000E+000
                7     0.0000000E+000 - 1.0000000E+003     7.5000000E+002
                8     5.0000000E+002 - 1.0000000E+003     7.5000000E+002
                9     0.0000000E+000 - 2.0000000E+003     0.0000000E+000
                10     5.0000000E+002 - 2.0000000E+003     0.0000000E+000
                11     0.0000000E+000 - 2.0000000E+003     7.5000000E+002
                12     5.0000000E+002 - 2.0000000E+003     7.5000000E+002
        */
    }

    return file_base;
}

std::tuple<int, int, int, int> VisageDeckWritter::get_element_indices( int element, int* cells )
{
    int c_k = int( element / (cells[0] * cells[1]) ),
        c_ij = element - c_k * (cells[0] * cells[1]),
        cell_j = int( c_ij / (cells[0]) ),
        cell_i = c_ij - cell_j * cells[0];

    return std::make_tuple( c_k, c_ij, cell_i, cell_j );
}

std::tuple<int, int, int, int> VisageDeckWritter::get_node_indices_for_element( int element, int* nodes )
{
    int c_k, c_ij, cell_i, cell_j;//, cell_k;
    int cells[] = { nodes[0] - 1,nodes[1] - 1,nodes[2] - 1 };
    tie( c_k, c_ij, cell_i, cell_j ) = get_element_indices( element, &cells[0] );

    int n1 = cell_j * nodes[0] + cell_i + c_k * (nodes[0] * nodes[1]),
        n2 = n1 + 1,
        n3 = n2 + nodes[0],
        n4 = n1 + nodes[0];

    return std::make_tuple( n1, n2, n3, n4 );
}

std::string VisageDeckWritter::write_ele_files( const VisageDeckSimulationOptions* options, const vector<float>* dvt_id, vector<int>* ele_pinch )// requires geometry descripotion (geometry size)
{
    std::string base_name = get_filename( options->step( ), options->model_name( ), "ele" );
    std::string file_name = (filesystem::path( options->path( ) ) /= filesystem::path( base_name )).string( ); ;
    //std::string debug_file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ) + "_debug_", "ele" );

    vector<int> dvt_index( options->geometry( ).num_cells( ), 0 ); //1-default 
    if(dvt_id != NULL) { copy( dvt_id->begin( ), dvt_id->end( ), dvt_index.begin( ) ); }

    ofstream file( file_name );
    if(!file) return "";

    //ofstream debug_file( debug_file_name );
    //if(!debug_file) return false;

    file << get_header( ) << std::endl << "*TOPOLOGY,NOCOM" << std::endl;

    auto geometry = options->geometry_size( );
    int nodes[3] = { geometry->num_nodes_i,geometry->num_nodes_j, geometry->num_nodes_k };
    int cells[3] = { geometry->num_nodes_i - 1,geometry->num_nodes_j - 1, geometry->num_nodes_k - 1 };
    int n_cells = cells[0] * cells[1] * cells[2];
    int element_shape = 13;
    int offset = nodes[0] * nodes[1];

    /*new version. We use the parameter t check the ordering */
    int n1, n2, n3, n4;
    for(int element = 0; element < n_cells; element++)
    {

        int dvt = 1 + dvt_index[element];
        std::tie( n1, n4, n3, n2 ) = get_node_indices_for_element( element, &nodes[0] );

        int tot_pinched = ele_pinch->at( element );
        //debug_file << 1 + element << "pinched nodes" << tot_pinched << " ele opuinch "<< ele_pinch->at( element )<< std::endl;

        int activity_code = tot_pinched >= 3 ? 2 : 0,
            zero_volume = tot_pinched >= 4 ? 1 : 0;

        file << 1 + element << '\t' << element_shape << '\t' << element + 1 << "    " << activity_code << "     " << zero_volume << "     " << dvt << "     " << std::endl;
        file << 1 + n2 << '\t' << 1 + n1 << '\t' << 1 + n4 << '\t' << 1 + n3 << '\t';//1 2 4 3
        file << offset + n2 + 1 << '\t' << offset + n1 + 1 << '\t' << offset + n4 + 1 << '\t' << offset + n3 + 1 << std::endl;
    }


    file.close( );
    //debug_file.close( );
    return base_name;
}

//bool VisageDeckWritter::write_ele_filesOLD( const VisageDeckSimulationOptions* options, bool flip_clock_wiseness, const vector<int>* activity, const vector<float>* dvt_id, vector<bool>* nodes_pinched_out = nullptr )// requires geometry descripotion (geometry size)
//{
//    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "ele" );
//    std::string debug_file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ) + "_debug_", "ele" );
//
//    vector<int> pinchout_nodes_count( options->geometry( ).num_cells( ), 0 ); //0-active 2-totally inactive (flow and stress)
//    if(activity != NULL) { copy( activity->begin( ), activity->end( ), pinchout_nodes_count.begin( ) ); }
//
//    vector<int> dvt_index( options->geometry( ).num_cells( ), 0 ); //1-default 
//    if(dvt_id != NULL) { copy( dvt_id->begin( ), dvt_id->end( ), dvt_index.begin( ) ); }
//
//    ofstream file( file_name );
//    if(!file) return false;
//
//    ofstream debug_file( debug_file_name );
//    if(!debug_file) return false;
//
//    file << get_header( ) << std::endl << "*TOPOLOGY,NOCOM" << std::endl;
//
//    auto geometry = options->geometry_size( );
//    int nodes[3] = { geometry->num_nodes_i,geometry->num_nodes_j, geometry->num_nodes_k };
//    int cells[3] = { geometry->num_nodes_i - 1,geometry->num_nodes_j - 1, geometry->num_nodes_k - 1 };
//    int n_cells = cells[0] * cells[1] * cells[2];
//    int element_shape = 13;
//    int offset = nodes[0] * nodes[1];
//
//    /*new version. We use the parameter t check the ordering */
//    int n1, n2, n3, n4;
//    int aux[] = { 0,0,0,0, 0,0,0,0 };
//    for(int element = 0; element < n_cells; element++)
//    {
//        int activity_code = pinchout_nodes_count[element] >= 3 ? 2 : 0,
//            zero_volume = pinchout_nodes_count[element] >= 4 ? 1 : 0;
//
//        int dvt = 1 + dvt_index[element];
//        file << 1 + element << '\t' << element_shape << '\t' << element + 1 << "    " << activity_code << "     " << zero_volume << "     " << dvt << "     " << std::endl;
//        std::tie( n1, n4, n3, n2 ) = get_node_indices_for_element( element, &nodes[0] );
//
//        debug_file << 1 + element << "pinched nodes" << pinchout_nodes_count[element] << std::endl;
//
// 
//
//        file << 1 + n2 << '\t' << 1 + n1 << '\t' << 1 + n4 << '\t' << 1 + n3 << '\t';//1 2 4 3
//        file << offset + n2 + 1 << '\t' << offset + n1 + 1 << '\t' << offset + n4 + 1 << '\t' << offset + n3 + 1 << std::endl;
//    }
//
//
//    file.close( );
//    debug_file.close( );
//    return true;
//}
//

//int VisageDeckWritter::write_fix_files( VisageDeckSimulationOptions* options, std::vector<float>* xyz ) //requires boundary comnditoions
//{
//    int fixities = 0;
//    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "fix" );
//    ofstream file( file_name );
//    if(!file)
//        return 0;
//    else
//    {
//        file << get_header( ) << std::endl;
//        file << "*CONSTRAINTS" << std::endl;
//
//        int vertical_dir = 2;
//        int i_dir = 1, j_dir = 3;
//
//        //the base will be fixed in the vertical direction which is 2;
//        //we assume that the nodes are starting from the top.
//        //lets check:
//        float z1 = xyz->at( 2 ), z2 = xyz->at( xyz->size( ) - 1 );
//        int ncols = options->geometry_size( )->num_nodes_i,
//            nrows = options->geometry_size( )->num_nodes_j,
//            nlayers = options->geometry_size( )->num_nodes_k,
//            total = ncols * nrows, start_index = 0;
//
//        if(z1 > z2) //nodes are starting from the top, so the base is actually the last part of the array
//        {
//            start_index = (int)xyz->size( ) - total;
//        }
//
//        //this is the base. Fix it in direction vertical_dir
//        //file << "--this is the base" << std::endl;
//        for(int n = start_index; n < start_index + total; n++)
//        {
//            file << n + 1 << '\t'; for(int d = 1; d <= 3; d++) file << (d == vertical_dir ? 1 : 0) << '\t'; file << std::endl; fixities += 1;
//        }
//
//        //file << "--these are the sides" << std::endl;
//        //lets go to the faces on the sides. we have direction i and direction j. i is the one varying fastest
//        int n = 0;
//        for(int k = 0; k < nlayers; k++)
//        {
//            for(int j = 0; j < nrows; j++)
//            {
//                for(int i = 0; i < ncols; i++)
//                {
//                    if((j == 0) || (j == nrows - 1))
//                    {
//                        int dir = 3;
//                        file << n + 1 << '\t'; for(int d = 1; d <= 3; d++) file << (d == dir ? 1 : 0) << '\t'; file << std::endl; fixities += 1;
//                    }
//                    if((i == ncols) || (i == ncols - 1))
//                    {
//                        int dir = 1;
//                        file << n + 1 << '\t'; for(int d = 1; d <= 3; d++) file << (d == dir ? 1 : 0) << '\t'; file << std::endl; fixities += 1;
//                    }
//                    n += 1;
//                }
//            }
//        }
//    }
//
//    return fixities;
//
//    /*
//    *CONSTRAINTS
//        9    0    1    0
//        10    0    1    0
//        11    0    1    0
//        12    0    1    0
//    */
//}

std::pair<int, int> VisageDeckWritter::write_top_basement_boundary_conditions( VisageDeckSimulationOptions* options )
{
    //first lets do the base and the top (if conditions at the top);
    std::vector<int> nodes;
    int fix_lines = 0, dis_lines = 0;
    for(int d : {2, 3})
    {
        IBoundaryCondition* b = options->get_boundary_condition( d );
        if(b != nullptr)
        {
            //this array appears irrelevant !!!  
            nodes.resize( options->geometry( )->nodes_per_layer( ) );

            //if basement nodes are the first listed->1,2,3... else nodes_per layer, nodes_per_layer + 1 ...
            std::iota( begin( nodes ), end( nodes ), 0 );

            BoundaryConditionType type = b->type( );
            switch(type)
            {
            case BoundaryConditionType::FIXEDSURFACE: //this will be eliminated
            {
                std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "fix" );
                ofstream fix_file( file_name, ios::app );

                for(auto i : nodes) fix_file << 1 + i << "\t0    1    0 " << "\t" << std::endl;

                fix_lines += (int)(nodes.size( ));

                fix_file.close( );
                break;
            }
            case BoundaryConditionType::DISPLACEMETSURFACE:
            {
                vector<float> displacements = static_cast<DisplacementSurfaceBoundaryCondition*>(b)->get_node_displacement( );

                if(displacements.size( ) <= 0) //it is a fix!!
                {
                    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "fix" );
                    ofstream fix_file( file_name, ios::app );

                    for(auto i : nodes) fix_file << 1 + i << "\t0    1    0 " << "\t" << std::endl;
                    fix_lines += (int)(nodes.size( ));

                    fix_file.close( );
                    break;
                }
                else //it is a disp!!
                {
                    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "dis" );
                    ofstream dis_file( file_name, ios::app );

                    int nn = 0;
                    for(auto i : nodes) dis_file << 1 + i << "  " << 2 << "  " << displacements[nn++] << std::endl;
                    dis_lines += (int)(nodes.size( ));

                    dis_file.close( );
                    break;
                }
            }

            default:
            {
                throw "Not implemented boundary condition";
            }
            }
        }
    }
    return  pair<int, int>( fix_lines, dis_lines );// (fix_lines, dis_lines);
}

std::pair<int, int> VisageDeckWritter::write_sides_boundary_conditions( VisageDeckSimulationOptions* options )
{
    //now lets do the sides.
    std::vector< std::vector<int> > side_nodes;
    options->geometry( )->get_side_node_indices( side_nodes ); //[ Ileft, IRight, jLeft, JRight ]

    std::vector<int> nodes;
    int fix_lines = 0, dis_lines = 0;
    for(int d : {0, 1})
    {
        IBoundaryCondition* b = options->get_boundary_condition( d );
        if(b != nullptr)
        {
            int dir = get_visage_dir( d );
            float sign = 1.0;// d == 0 ? 1.0 : -1.0;
            BoundaryConditionType type = b->type( );
            switch(type)
            {
            case BoundaryConditionType::STRAIN:
            {
                float length = options->geometry( )->length( )[d];
                float displacement = sign * length * static_cast<StrainBoundaryCondition*>(b)->strain( );

                std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "dis" );
                ofstream dis_file( file_name, ios::app );

                std::vector<int>& vec1 = side_nodes[2 * d];
                for(auto i : vec1) dis_file << 1 + i << "  " << dir << setprecision( 10 ) << "  " << displacement << std::endl;

                std::vector<int>& vec2 = side_nodes[2 * d + 1];
                for(auto i : vec2) dis_file << 1 + i << "  " << dir << "   " << setprecision( 10 ) << -displacement << std::endl;

                dis_file.close( );
                dis_lines += (int)(vec1.size( ) + vec2.size( ));
                break;
            }

            default:
            {
                throw "Not implememted condition for the sides. Only strain for now";
            }
            }
        }//not null
    }//d

    return  pair<int, int>( 0, dis_lines );
}

std::pair<std::string, std::string> VisageDeckWritter::write_boundary_conditions( VisageDeckSimulationOptions* options )
{
    //create the two files, the .dis and the .fix. Then we will append lines to them,
    std::string extensions[] = { "fix", "dis" }, headers[] = { "*CONSTRAINTS ", "*DISPLACEMENTS, N " };
    std::string files[2];
    for(auto d : { 0,1 })
    {
        std::string base_name = get_filename( options->step( ), options->model_name( ), extensions[d] );
        files[d] = base_name;
        std::string file_name = (filesystem::path( options->path( ) ) /= filesystem::path( base_name )).string( );// + "\\" + get_filename( options->step( ), options->model_name( ), extensions[d] );
        ofstream file( file_name, ios::out );
        if(!file) return pair<std::string, std::string>( "", "" );
        file << headers[d] << std::endl;
        file.close( );
    }

    pair<int, int> top_base_fixities = write_top_basement_boundary_conditions( options );

    pair<int, int> sides_fixities = write_sides_boundary_conditions( options );

    pair<int, int> fix_dis( top_base_fixities.first + sides_fixities.first, top_base_fixities.second + sides_fixities.second );

    options->set_replace_instruction( "HEADER", "Nconstraints", std::to_string( fix_dis.first ) );
    options->set_replace_instruction( "HEADER", "Ndisplacements", std::to_string( fix_dis.second ) );
    options->set_replace_instruction( "HEADER", "Sdisplacements", fix_dis.second > 0 ? "1" : "0" );

    return  pair<string, string>( files[0], files[1] );// fix_dis;
}

std::string VisageDeckWritter::write_deck( VisageDeckSimulationOptions* options, ArrayData* arrays, const map<std::string, float>* unit_conversion, bool dummy )
{
    StructuredGrid& geometry = options->geometry( );
    CoordinateMapping3D geometry_reference = geometry->reference( );

    vector<fVector3> all_coordinates;

    //dont flip
    for(int n : IntRange( 0, geometry->nsurfaces( ) ))
    {
        vector<fVector3> surface_coordinates = geometry->get_local_coordinates_vector( n );
        copy( begin( surface_coordinates ), end( surface_coordinates ), back_inserter( all_coordinates ) );
    }


    std::vector<float> transformed_coordinates;
    for(fVector3& v : all_coordinates)
    {
        for(auto d : IntRange( 0, 3 ))
            transformed_coordinates.push_back( v[d] );
    }

    CoordinateMapping3D axis_aligned_reference( fVector3( 1.0, 0.0, 0.0 ), fVector3( 0.0, 1.0, 0.0 ), fvector3( 0.0, 0.0, 1.0 ), geometry_reference.origin );
    geometry_reference.convert_to( axis_aligned_reference, transformed_coordinates );


    const CoordinateMapping3D visage_reference( fvector3( 1.0, 0.0, 0 ), fvector3( 0.0, 0.0, 1.0 ), fvector3( 0.0, 1.0, 0.0 ) );
    axis_aligned_reference.convert_to( visage_reference, transformed_coordinates );


    if(dummy)
    {
        std::string nquick_clac = options->get_command( "HEADER" )->at( "Nquickcalculation" );
        std::string n_iter = options->get_command( "HEADER" )->at( "Niterations" );
        std::string n_increments = options->get_command( "HEADER" )->at( "Nsub_increments" );
        options->set_replace_instruction( "HEADER", "Nquickcalculation", "1" );
        options->set_replace_instruction( "HEADER", "Niterations", "1" );
        options->set_replace_instruction( "HEADER", "Nsub_increments", "1" );

        std::string mii = VisageDeckWritter::write_deck( options, &transformed_coordinates, arrays, unit_conversion );

        options->set_replace_instruction( "HEADER", "Nquickcalculation", nquick_clac );
        options->set_replace_instruction( "HEADER", "Niterations", n_iter );
        options->set_replace_instruction( "HEADER", "Nsub_increments", n_increments );
        return mii;
    }

    else
        return   VisageDeckWritter::write_deck( options, &transformed_coordinates, arrays, unit_conversion );

}

//std::string VisageDeckWritter::write_deck( VisageDeckSimulationOptions* options, ArrayData* arrays, const map<std::string, float>* unit_conversion )
//{
//    StructuredGrid& geometry = options->geometry( );
//    CoordinateMapping3D geometry_reference = geometry->reference( );
//
//    vector<fVector3> all_coordinates;
//
//    //dont flip
//    for(int n : IntRange( 0, geometry->nsurfaces( ) ))
//    {
//        vector<fVector3> surface_coordinates = geometry->get_local_coordinates_vector( n );
//        copy( begin( surface_coordinates ), end( surface_coordinates ), back_inserter( all_coordinates ) );
//    }
//
//    CoordinateMapping3D axis_aligned_reference( fVector3( 1.0, 0.0, 0.0 ), fVector3( 0.0, 1.0, 0.0 ), fvector3( 0.0, 0.0, 1.0 ), geometry_reference.origin );
//    std::vector<float> transformed_coordinates;
//    for(fVector3& v : all_coordinates)
//    {
//        for(auto d : IntRange( 0, 3 ))
//            transformed_coordinates.push_back( v[d] );
//    }
//
//    const CoordinateMapping3D visage_reference( fvector3( 1.0, 0.0, 0 ), fvector3( 0.0, 0.0, 1.0 ), fvector3( 0.0, 1.0, 0.0 ) );
//    geometry_reference.convert_to( visage_reference, transformed_coordinates );
//
//    return   VisageDeckWritter::write_deck( options, &transformed_coordinates, arrays, unit_conversion );
//}



std::string VisageDeckWritter::write_deck( VisageDeckSimulationOptions* options, std::vector<float>* xyz, ArrayData* arrays, const map<std::string, float>* unit_conversion )
{
    vector<std::string> include_files;

    //dvt tables
    const vector<float>* dvt_id = nullptr;//zero-based 
    if(optional <pair<int, std::string> > dvt_info = write_dvt_tables( options, arrays ); dvt_info.has_value( ))
    {
        include_files.push_back( dvt_info.value( ).second );
        dvt_id = &(arrays->at( "dvt_table_index" ));
    }

    //fix,dis
    auto [fix_file, dis_file] = write_boundary_conditions( options );
    include_files.push_back( fix_file );
    include_files.push_back( dis_file );


    //materials 
    if(std::string mat_file = write_mat_files( options, arrays, unit_conversion ); !mat_file.empty( ))
    {
        include_files.push_back( mat_file );
    }
    cout << "Mat files done" << std::endl;


    //.nod 
    if(std::string node_file = write_node_files( options, xyz ); !node_file.empty( ))
    {
        include_files.push_back( node_file );
    }
    cout << "Node files done" << std::endl;
    //.connect 
    vector<int>  element_pinched_count;
    map<int, int> node_connections;
    //this creates a list of pairs <node1, node2>,....<node1,node3>,,...<node1,node10>....etc  
    //but we found out that actually visage needs the whole list of nodes in one line. The followinf lines construct the tree of connections  
    options->geometry( )->brute_force_get_pinched_elements( options->pinchout_tolerance( ), element_pinched_count, node_connections );
    if(std::string connect_file = write_connections_files2( node_connections, options ); !connect_file.empty( ))
    {
        include_files.push_back( connect_file );
    }
    cout << "Connect files done" << std::endl;

    //.ele 
    if(std::string ele_file = write_ele_files( options, dvt_id, &element_pinched_count ); !ele_file.empty( ))
    {
        include_files.push_back( ele_file );
    }
    cout << "Ele files done" << std::endl;



    ////////////////////////////  BECAUSE CONNECTIONS ARE MESSED UP, WE ADDED THIS PART //////////
    //std::map<int, std::vector<int>> connections_path;
    //vector<bool>  nodes_pinched_out( options->geometry( )->total_nodes( ), false );
   // if(node_connections.size( ) > 0)
   // {
   //     std::vector<int> linked_connections( options->geometry( )->total_nodes( ) );
   //     std::iota( linked_connections.begin( ), linked_connections.end( ), 0 );

   //     for(const auto& pair : node_connections)
   //     {
   //         int i1 = pair.first, j1 = i1;
   //         int i2 = pair.second; //second, follows first, which follows j1 
   //         nodes_pinched_out[i1] = true;
   //         nodes_pinched_out[i2] = true;

   //         while(j1 != linked_connections[i1]) j1 = linked_connections[j1];

   //         linked_connections[i2] = j1;
   //         connections_path[j1].push_back( i2 );
   //     }


   //     std::string connect_file = write_connections_files( connections_path, options );
   //     include_files.push_back( connect_file );
   // }
   //////////////////////////////////////end of the part////////////////////////////////////////////


    if(arrays->id( ) == WellKnownVisageNames::FailureMode::ELASTIC)
    {
        cout << "Elastic parameters             " << endl;
        options->set_command( "ELASTIC" );
        options->set_replace_instruction( "RESULTS", "ele_tot_pl_strain", std::to_string( 0 ) );
        options->set_replace_instruction( "HEADER", "Nsub_increments", std::to_string( 1 ) );
    }
    else
    {
        cout << "Plastic parameters             " << endl;
        options->delete_command( "ELASTIC" );
        options->set_replace_instruction( "HEADER", "Nsub_increments", std::to_string( 10 ) );
        options->set_replace_instruction( "RESULTS", "ele_tot_pl_strain", std::to_string( 1 ) );
    }


    cout << "Configuring solver " << std::endl;
    cout << boolalpha << " Auto-config solver enabled ? " << options->auto_config_solver( ) << std::endl;
    if(options->auto_config_solver( ))
    {
        auto& v = arrays->get_array( "COHESION" );
        float avg_cohesion = accumulate( v.begin( ), v.end( ), 0.0f ) / v.size( );
        options->set_replace_instruction( "HEADER", "vyieldtolerance", std::to_string( 250.0f ) );// 0.1 * avg_cohesion ) );
        //FIXME: hard-coded reasonabe value.
        int n_nodes = 1 + (int)(0.005 * 8 * xyz->size( ) / 3);
        options->set_replace_instruction( "HEADER", "nyield_gp_number", std::to_string( n_nodes ) );
        options->set_replace_instruction( "HEADER", "Nquickcalculation", std::to_string( 50 ) );
        options->set_replace_instruction( "HEADER", "Niterations", std::to_string( 50 ) );
    }

    return write_mii_files( options, include_files );
}
