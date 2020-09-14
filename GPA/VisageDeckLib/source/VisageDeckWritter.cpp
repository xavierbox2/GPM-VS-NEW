#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif

#include <array>
#include <algorithm>
#include <limits>
#include <numeric>

#include <optional>

#include "Range.h"
#include "Vector3.h"
#include "ArrayData.h"
#include "VisageDeckWritter.h"

using namespace std;

std::optional< pair<int, string> >VisageDeckWritter::write_dvt_tables( VisageDeckSimulationOptions* options, ArrayData* arrays )
{
    if((arrays == nullptr) || (!arrays->contains( "dvt_table_index" ) || (!options->use_tables( )) || (options->_tables.size( ) <= 0)))
    {
        options->set_replace_instruction( "RESULTS", "ele_dvt_variation", "0" );
        options->set_replace_instruction( "HEADER", "ndvttables", "0" );
        return optional<  pair<int, string> >{};
    }

    string file_base = get_filename( options->step( ), options->model_name( ), "dvt" );
    string file_name = options->path( ) + "\\" + file_base;
    ofstream file( file_name );

    bool write_header = true;

    //table with the highest resolution 
    auto it = std::max_element( options->_tables.begin( ), options->_tables.end(), []( const auto &p1, const auto &p2){ return p1.second.size() < p2.second.size(); } );
    int table_size = it->second.size();


    //int table_size = -1;
    for(auto& p : options->_tables)
    {
        Table& t = p.second;

        if(write_header)
        {
            file << "*DVTTABLES" << std::endl;
            file << options->_tables.size( ) << " " << t.size( ) << endl;
            //table_size = t.size( );
            write_header = false;
        }

        t.resample( table_size );

        int index = p.first + 1;
        file << t.controller_var_name( ) << " " << t.dependant_name( ) << endl;
        for(const auto& pair : t.value_pairs( ))
            file << pair.first << " " << pair.second << endl;

        file << endl;
    }
    file.close( );

    options->set_replace_instruction( "RESULTS", "ele_dvt_variation", "1" );
    options->set_replace_instruction( "HEADER", "ndvttables", to_string( options->_tables.size( ) ) );//to_string(options->_tables.size()));


    return optional<  pair<int, string> > { make_pair( options->_tables.size( ), file_base ) };
}

/*
std::pair<int, string> VisageDeckWritter::write_dvt_tablesold(VisageDeckSimulationOptions *options)
{
    int n_tables = 0;
    string file_base = "", file_name = "";

    if ((options->use_tables()) && (options->_tables.size() > 0))
    {
        file_base = get_filename(options->step(), options->model_name(), "dvt");
        file_name = options->path() + "\\" + file_base;

        ofstream file(file_name);
        file << "*DVTTABLES" << std::endl;
        Table &t = options->_tables.at(1);
        file << "1" << " " << t.size() << endl << endl;

        file << t.controller_var_name << " " << t.dependant_name << endl;
        for (auto& pair : t.value_pairs)
            file << pair.first << " " << pair.second << endl;

        file.close();
        n_tables = 1;
        options->SetOrReplaceHash("RESULTS", "ele_dvt_variation", "1");
        options->SetOrReplaceHash("HEADER", "ndvttables", "1");//to_string(options->_tables.size()));
    }
    else
    {
        options->SetOrReplaceHash("RESULTS", "ele_dvt_variation", "0");
        options->SetOrReplaceHash("HEADER", "ndvttables", "0");
    }

    return make_pair(n_tables, file_base);
}*/

std::string VisageDeckWritter::write_mii_files( VisageDeckSimulationOptions* options, vector<std::string>& files ) //requires required path model names options and geometry dfescripotion (geometry size)
{
    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "MII" );
    std::string text = options->to_string( );
    ofstream file( file_name );
    if(!file)
        return "";
    else
    {
        /*
        //if the order didnt matter, this would be all we would need. However, it apparently matters.
        //so we will haveto take the NOECHO and HEADER and write thm first. Then we write the rest.
        file << get_header() << std::endl;
        file << text << std::endl;
        file.close();
        */

        std::vector< InstructionBlock* > commands = options->get_commands( );

        auto echo = options->get_command( "NOECHO" );
        auto header = options->get_command( "HEADER" );
        auto name = options->get_command( "MODELNAME" );

        std::string s = "";
        if(name != NULL)
        {
            s += name->to_string( ) + "\n";
        }
        if(echo != NULL)
        {
            s += echo->to_string( ) + "\n";
        }
        if(header != NULL)
        {
            s += header->to_string( ) + "\n";
        }


        for(int n = 0; n < commands.size( ); n++)
        {
            if((commands[n]->Name != "NOECHO") && (commands[n]->Name != "HEADER") && (commands[n]->Name != "MODELNAME"))
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
    }

    return file_name;
}

void write_property_array( ofstream& file, int grid_size, string vs_keyword, const vector<float>& phi, float scale = 1.0f )
{
    file << vs_keyword << std::endl;

    if(phi.size( ) == 1)
    {
        file << grid_size << " [   " << phi[0] *scale << " ] " << endl;
    }

    else
    {
        for(int n = 0; n < grid_size; n++) file << " " << phi[n] * scale << endl;
    }

    file << endl;
}

string VisageDeckWritter::write_plastic_mat_file( VisageDeckSimulationOptions* options, ArrayData* data, const map<string, float>* unit_conversion )
{
    function<float( string, const map<string, float>* )> units = []( string s, const map<string, float>* unit )
    { return unit != nullptr && unit->find( s ) != unit->end( ) ? unit->at( s ) : 1.0f; };


    std::string mat_file = get_filename( options->step( ), options->model_name( ), "mat" );
    std::string file_name = options->path( ) + "\\" + mat_file;
    ofstream file( file_name );
    if(!file)
        return "";

    int grid_size = options->geometry( )->num_cells( );

    if((data->contains( "YOUNGMOD" )) && (data->contains( "POISSONR" )))
    {
        vector<float>& ym = data->get_array( "YOUNGMOD" );
        vector<float>& pr = data->get_array( "POISSONR" );
        file << "*ELASTIC_DATA,NOCOM" << std::endl;

        float scale1 = units( "YOUNGMOD", unit_conversion ), scale2 = units( "POISSONR", unit_conversion );
        size_t mult_values = (ym.size( ) > 1) && (pr.size( ) > 1) ? 1 : 0;
        for(size_t n = 0; n < grid_size; n++) file << " 1  " << ym[n * mult_values] * scale1 << " " << pr[n * mult_values] * scale2 << endl;
        file << endl;
    }

    {
        vector<float>& phi = data->get_or_create_array( "POROSITY", 0.3f, 1 );
        write_property_array( file, grid_size, "*POROSITY, NOCOM", phi );
    }
    {
        vector<float>& biot = data->get_or_create_array( "BIOTC", 1.0, 1 );//.data();
        write_property_array( file, grid_size, "*BIOTS_MODULUS,NOCOM", biot );
    }

    if(data->contains( "DENSITY" ))
    {
        vector<float>& phi = data->get_array( "DENSITY" );
        float scale1 = units( "DENSITY", unit_conversion );
        write_property_array( file, grid_size, "*SOLID_UNIT_W,NOCOM", phi, scale1 );
    }

    if((!options->enforce_elastic( )) && (data->contains( "COHESION" )))
    {
        vector< pair<string, float> > defaults
        {
            {"COHESION",50.0},
            {"FRICTION",30.0},
            {"DILATION",15.0},
            {"FLUIDITY",1.0},
            {"HARDENING",-0.005},
            {"TENSILE_STRENGTH",5.0e3},
            {"MAX_COHESION_CHANGE",50.0}
        };
        vector<bool> had_names( defaults.size( ), true );
        for(auto n : IntRange( 0, (int)defaults.size( ) ))
        {
            if(!data->contains( defaults[n].first ))
            {
                had_names[n] = false;
                data->get_or_create_array( defaults[n].first, defaults[n].second, grid_size );
            }
        }

        //string names[] = { "COHESION","FRICTION", "DILATION","FLUIDITY","HARDENING","TESILE_STRENGTH", "MAX_COHESION_CHANGE" };
        //vector<float> default_values{ 1e3f, 30.0f, 15.0f, 1.0f,  0.0f, 1.0e2f, 100.0 };

        vector<float>& cohesion = data->get_array( "COHESION" );
        vector<float>& friction = data->get_array( "FRICTION" );
        vector<float>& dilation = data->get_array( "DILATION" );

        vector<float>& fluidity = data->get_array( "FLUIDITY" );
        vector<float>& hardening = data->get_array( "HARDENING" );
        vector<float>& tensile = data->get_array( "TENSILE_STRENGTH" );
        vector<float>& coh_change = data->get_array( "MAX_COHESION_CHANGE" );

        file << "*YIELD_DATA,NOCOM" << std::endl;

        float coh_scale_factor = unit_conversion != nullptr && unit_conversion->find( "COHESION" ) != unit_conversion->end( ) ?
            unit_conversion->at( "COHESION" ) : 1.0f;

        float tens_scale_factor = unit_conversion != nullptr && unit_conversion->find( "TENSILE_STRENGTH" ) != unit_conversion->end( ) ?
            unit_conversion->at( "TENSILE_STRENGTH" ) : 1.0f;

        if(cohesion.size( ) == 1)
        {
            for(int n = 0; n < grid_size; n++)
            {
                file << -1 << " " << coh_scale_factor * cohesion[0] << " " << friction[0] << " " << dilation[0] << " " << fluidity[0] << " " << hardening[0] << " " << tens_scale_factor * tensile[0] << " " << coh_scale_factor * coh_change[0] << endl;
            }
        }
        else
        {
            for(int n = 0; n < grid_size; n++)
            {
                file << -1 << " " << coh_scale_factor * cohesion[n] << " " << friction[n] << " " << dilation[n] << " " << fluidity[n] << " " << hardening[n] << " " << tens_scale_factor * tensile[n] << " " << coh_scale_factor * cohesion[n] * 0.9 <<endl;//coh_scale_factor * coh_change[n] << endl;
            }
        }

        file << endl;

        //delete the arrays that werent part of the initial dataset
        for(auto n : IntRange( 0, (int)defaults.size( ) ))
        {
            if(!had_names[n]) data->delete_array( defaults[n].first );
        }
    }

    file.close( );

    return mat_file;
}

string VisageDeckWritter::write_mat_files( VisageDeckSimulationOptions* options, ArrayData* arrays, const map<string, float>* unit_conversion ) //requires required path model names options and geometry dfescripotion (geometry size)
{
    if(arrays == NULL) return false;

    if((options->enforce_elastic( )) || (!arrays->contains( "COHESION" )))
    {
        options->set_command( "ELASTIC" );
        options->delete_instruction( "RESULTS", "ele_tot_pl_strain" );
    }
    else
    {
        options->delete_command( "ELASTIC" );
        options->set_replace_instruction( "RESULTS", "ele_tot_pl_strain", "1" );
    }

    return write_plastic_mat_file( options, arrays, unit_conversion );
}

//we assume that the elements are top-down.
pair<int, string> VisageDeckWritter::write_edge_loads( VisageDeckSimulationOptions* options, std::vector<float>* xyz )
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

    string file_name = get_filename( options->step( ), options->model_name( ), "edg" );
    string file_path = options->path( ) + "\\" + file_name;
    ofstream file( file_path );
    if(!file) return pair<int, string>( 0, file_path );

    if((xyz == NULL) || (options->sea_level( ) == -999.0)) return pair<int, string>( 0, file_name );

    int total_loads = 0;
    vector<int> indices;
    vector<float> loads;
    total_loads = get_edge_loads( options, xyz, indices, loads );

    if(total_loads < 1)
    {
        file.close( );
        return pair<int, string>( 0, file_name );
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

    return pair<int, string>( total_loads, file_name );
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

string  VisageDeckWritter::write_connections_files( map<int, int>& connections, VisageDeckSimulationOptions* options )
{
    std::string file_base = get_filename( options->step( ), options->model_name( ), "rcn" );
    std::string file_name = options->path( ) + "\\" + file_base;
    ofstream file( file_name );
    file << "*CONNECT" << std::endl << std::endl;
    for(pair<const int, int>& p : connections)
    {
        //int node1 = n1 + node_offset;
        file << "		 2 1110000000" << endl;
        file << 1 + p.first << "        " << 1 + p.second << endl;
    }

    file.close( );

    options->set_replace_instruction( "HEADER", "Sconnect", (connections.size( ) > 0 ? " 1 " : " 0 ") );
    options->set_replace_instruction( "HEADER", "Nconnect", std::to_string( connections.size( ) ) );

    return file_base;
}

//requires the explicit node coordinates in the visage reference frame
string VisageDeckWritter::write_node_files( VisageDeckSimulationOptions* options, std::vector<float>* xyz )//, bool &is_clock_wise)
{
    if(xyz == NULL) return false;

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
        /*--Shifted by 0.0000000E+000 0.0000000E+000 7.5000000E+002 from the reservoir model
                *COORDS, NOCOM
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

        //lets figure out if the first face to be written by write_ele would have the righ orientation
        //int n1, n2, n3, n4;
        //auto geometry = options->geometry_size();
        //int nodes[3] = { geometry->num_nodes_i,geometry->num_nodes_j, geometry->num_nodes_k };
        //tie(n1, n2, n3, n4) = get_node_indices_for_element(0, nodes);

        //{
        //    fvector3 v1(xyz->at(3 * n1 + 0), xyz->at(3 * n1 + 1), xyz->at(3 * n1 + 2));
        //    fvector3 v21 = fvector3(xyz->at(3 * n2 + 0), xyz->at(3 * n2 + 1), xyz->at(3 * n2 + 2)) - v1; //x'
        //    fvector3 v31 = (fvector3(xyz->at(3 * n3 + 0), xyz->at(3 * n3 + 1), xyz->at(3 * n3 + 2))) - v1;// - v2; //y'
        //    //fvector3 v4 = (fvector3(xyz->at(3 * n4 + 0), xyz->at(3 * n4 + 1), xyz->at(3 * n4 + 2)));// - v1) - v2; //y'

        //    fvector3 xprime = (v21).normalize();
        //    fvector3 zprime = (v31).normalize().cross(xprime);
        //    fvector3 yprime = zprime.cross(xprime);

        //    v1 = xprime * (v21.dot(xprime));
        //    fVector3 v2 = fvector3(v31.dot(xprime), v31.dot(yprime), v31.dot(zprime));

        //    float A = v1.cross(v2).trace();
        //    is_clock_wise = (A >= 0.0f ? false : true);
        //}

        //{
        //    fvector3 v1(xyz->at(3 * n1 + 0), xyz->at(3 * n1 + 1), xyz->at(3 * n1 + 2));
        //    fvector3 v4 = fvector3(xyz->at(3 * n2 + 0), xyz->at(3 * n2 + 1), xyz->at(3 * n2 + 2));// - v1; //x'
        //    fvector3 v3 = (fvector3(xyz->at(3 * n3 + 0), xyz->at(3 * n3 + 1), xyz->at(3 * n3 + 2)));// - v1);// - v2; //y'
        //    fvector3 v2 = (fvector3(xyz->at(3 * n4 + 0), xyz->at(3 * n4 + 1), xyz->at(3 * n4 + 2)));// - v1) - v2; //y'

        //    fvector3 xprime = (v2 - v1).normalize();
        //    fvector3 zprime = (v3 - v1).normalize().cross(xprime);
        //    fvector3 yprime = zprime.cross(xprime);

        //    v1 = xprime * ((v2 - v1).dot(xprime));// fvector3(xprime, yprime, zprime));
        //    v2 = fvector3((v3 - v1).dot(xprime), (v3 - v1).dot(yprime), (v3 - v1).dot(zprime));

        //    float A = v1.cross(v2).trace();
        //  //  cout << A;
        //}
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

bool VisageDeckWritter::write_ele_files( const VisageDeckSimulationOptions* options, bool flip_clock_wiseness, const vector<int>* activity, const vector<float>* dvt_id )// requires geometry descripotion (geometry size)
{
    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "ele" );

    vector<int> activity_flag( options->geometry( ).num_cells( ), 0 ); //0-active 2-totally inactive (flow and stress)
    if(activity != NULL) { copy( activity->begin( ), activity->end( ), activity_flag.begin( ) ); }

    vector<int> dvt_index( options->geometry( ).num_cells( ), 0 ); //1-default 
    if(dvt_id != NULL) { copy( dvt_id->begin( ), dvt_id->end( ), dvt_index.begin( ) ); }

    ofstream file( file_name );
    if(!file) return false;

    file << get_header( ) << std::endl << "*TOPOLOGY,NOCOM" << std::endl;

    auto geometry = options->geometry_size( );
    int nodes[3] = { geometry->num_nodes_i,geometry->num_nodes_j, geometry->num_nodes_k };
    int cells[3] = { geometry->num_nodes_i - 1,geometry->num_nodes_j - 1, geometry->num_nodes_k - 1 };
    int n_cells = cells[0] * cells[1] * cells[2];
    int element_shape = 13;
    int offset = nodes[0] * nodes[1];

    /*new version. We use the parameter t check the ordering */
    int n1, n2, n3, n4;
    if(flip_clock_wiseness) {
        for(int element = 0; element < n_cells; element++)
        {
            int activity_code = 0, param5 = 0;
            if(activity_flag[element] >= 4) { param5 = 1; activity_code = 2; }
            else if(activity_flag[element] == 3) { activity_code = 2; }
            else { ; }
            int dvt = 1 + dvt_index[element];

            file << 1 + element << '\t' << element_shape << '\t' << element + 1 << "    " << activity_code << "     " << param5 << "    " << dvt << "     " << std::endl;
            std::tie( n1, n2, n3, n4 ) = get_node_indices_for_element( element, &nodes[0] );

            file << 1 + n2 << '\t' << 1 + n1 << '\t' << 1 + n4 << '\t' << 1 + n3 << '\t';//1 2 4 3
            file << offset + n2 + 1 << '\t' << offset + n1 + 1 << '\t' << offset + n4 + 1 << '\t' << offset + n3 + 1 << std::endl;
        }
    }
    else
    {
        cout << "If i am here, then remove the flip_clock_wiseness flag !!!!" << endl;

        for(int element = 0; element < n_cells; element++)
        {
            int activity_code = 0, param5 = 0;
            if(activity_flag[element] >= 4) { param5 = 1; activity_code = 2; }
            else if(activity_flag[element] == 3) { activity_code = 2; }
            else { ; }

            int dvt = 1 + dvt_index[element];

            file << 1 + element << '\t' << element_shape << '\t' << element + 1 << "    " << activity_code << "     " << param5 << "     " << dvt << "     " << std::endl;
            std::tie( n1, n4, n3, n2 ) = get_node_indices_for_element( element, &nodes[0] );

            //old
            file << 1 + n2 << '\t' << 1 + n1 << '\t' << 1 + n4 << '\t' << 1 + n3 << '\t';//1 2 4 3
            file << offset + n2 + 1 << '\t' << offset + n1 + 1 << '\t' << offset + n4 + 1 << '\t' << offset + n3 + 1 << std::endl;
        }
    }

    file.close( );

    return true;
}

int VisageDeckWritter::write_fix_files( VisageDeckSimulationOptions* options, std::vector<float>* xyz ) //requires boundary comnditoions
{
    int fixities = 0;
    std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), "fix" );
    ofstream file( file_name );
    if(!file)
        return 0;
    else
    {
        file << get_header( ) << std::endl;
        file << "*CONSTRAINTS" << std::endl;

        int vertical_dir = 2;
        int i_dir = 1, j_dir = 3;

        //the base will be fixed in the vertical direction which is 2;
        //we assume that the nodes are starting from the top.
        //lets check:
        float z1 = xyz->at( 2 ), z2 = xyz->at( xyz->size( ) - 1 );
        int ncols = options->geometry_size( )->num_nodes_i,
            nrows = options->geometry_size( )->num_nodes_j,
            nlayers = options->geometry_size( )->num_nodes_k,
            total = ncols * nrows, start_index = 0;

        if(z1 > z2) //nodes are starting from the top, so the base is actually the last part of the array
        {
            start_index = (int)xyz->size( ) - total;
        }

        //this is the base. Fix it in direction vertical_dir
        //file << "--this is the base" << std::endl;
        for(int n = start_index; n < start_index + total; n++)
        {
            file << n + 1 << '\t'; for(int d = 1; d <= 3; d++) file << (d == vertical_dir ? 1 : 0) << '\t'; file << std::endl; fixities += 1;
        }

        //file << "--these are the sides" << std::endl;
        //lets go to the faces on the sides. we have direction i and direction j. i is the one varying fastest
        int n = 0;
        for(int k = 0; k < nlayers; k++)
        {
            for(int j = 0; j < nrows; j++)
            {
                for(int i = 0; i < ncols; i++)
                {
                    if((j == 0) || (j == nrows - 1))
                    {
                        int dir = 3;
                        file << n + 1 << '\t'; for(int d = 1; d <= 3; d++) file << (d == dir ? 1 : 0) << '\t'; file << std::endl; fixities += 1;
                    }
                    if((i == ncols) || (i == ncols - 1))
                    {
                        int dir = 1;
                        file << n + 1 << '\t'; for(int d = 1; d <= 3; d++) file << (d == dir ? 1 : 0) << '\t'; file << std::endl; fixities += 1;
                    }
                    n += 1;
                }
            }
        }
    }

    return fixities;

    /*
    *CONSTRAINTS
        9    0    1    0
        10    0    1    0
        11    0    1    0
        12    0    1    0
    */
}

std::pair<int, int> VisageDeckWritter::write_top_basement_boundary_conditions( VisageDeckSimulationOptions* options )
{
    //first lets do the base and the top (if conditions at the top);
    std::vector<int> nodes;
    int fix_lines = 0, dis_lines = 0;
    for(int d : {2, 3})
    {
        IBoundaryCondition* b = options->get_boundary_condition( d );
        if(b != NULL)
        {
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
        if(b != NULL)
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

std::pair<int, int> VisageDeckWritter::write_boundary_conditions( VisageDeckSimulationOptions* options )
{
    //create the two files, the .dis and the .fix. Then we will append lines to them,
    string extensions[] = { "fix", "dis" }, headers[] = { "*CONSTRAINTS ", "*DISPLACEMENTS, N " };
    for(auto d : { 0,1 })
    {
        std::string file_name = options->path( ) + "\\" + get_filename( options->step( ), options->model_name( ), extensions[d] );
        ofstream file( file_name, ios::out );
        if(!file) return pair<int, int>( 0, 0 );
        file << headers[d] << std::endl;
        file.close( );
    }

    pair<int, int> top_base_fixities = write_top_basement_boundary_conditions( options );

    pair<int, int> sides_fixities = write_sides_boundary_conditions( options );

    pair<int, int> fix_dis( top_base_fixities.first + sides_fixities.first, top_base_fixities.second + sides_fixities.second );

    options->set_replace_instruction( "HEADER", "Nconstraints", std::to_string( fix_dis.first ) );
    options->set_replace_instruction( "HEADER", "Ndisplacements", std::to_string( fix_dis.second ) );
    options->set_replace_instruction( "HEADER", "Sdisplacements", fix_dis.second > 0 ? "1" : "0" );

    return  fix_dis;
}

std::string VisageDeckWritter::write_deck( VisageDeckSimulationOptions* options, ArrayData* arrays, const map<string, float>* unit_conversion )
{
    StructuredGrid& geometry = options->geometry( );
    CoordinateMapping3D geometry_reference = geometry->reference( );

    //bool is_bottom_up = geometry->bottom_up();
    vector<fVector3> all_coordinates;
    ;

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

    const CoordinateMapping3D visage_reference( fvector3( 1.0, 0.0, 0 ), fvector3( 0.0, 0.0, 1.0 ), fvector3( 0.0, 1.0, 0.0 ) );
    geometry_reference.convert_to( visage_reference, transformed_coordinates );

    return   VisageDeckWritter::write_deck( options, &transformed_coordinates, arrays, unit_conversion );
}





std::string VisageDeckWritter::write_deck( VisageDeckSimulationOptions* options, std::vector<float>* xyz, ArrayData* arrays, const map<string, float>* unit_conversion )
{
    vector<std::string> include_files;

    //dvt tables
    optional <pair<int, string> > dvt_info = write_dvt_tables( options, arrays );
    if(dvt_info.has_value( ))
    {
        include_files.push_back( dvt_info.value( ).second );
    }

    //fix,dis
    write_boundary_conditions( options );
    for(string extension : {"fix", "dis"})
        include_files.push_back( get_filename( options->step( ), options->model_name( ), extension ) );

    if(arrays != NULL)
    {
        string mat_file = write_mat_files( options, arrays, unit_conversion );
        if(!mat_file.empty( ))
        {
            include_files.push_back( mat_file );
        }

        //int num_loads = 0;
        //string edge_file = "";
        //if(num_loads > 0)
        //{
        //    include_files.push_back( edge_file );
        //    options->set_replace_instruction( "HEADER", "Sedgeload", num_loads > 0 ? "1" : "0" );
        //}

        write_node_files( options, xyz );
        include_files.push_back( get_filename( options->step( ), options->model_name( ), "nod" ) );

        vector<int>  element_pinched_count;
        map<int, int> node_connections;
        options->geometry( )->brute_force_get_pinched_elements( options->pinchout_tolerance( ), element_pinched_count, node_connections );
        if(node_connections.size( ) > 0)
        {
            string connect_file = write_connections_files( node_connections, options );
            include_files.push_back( connect_file );
        }

        bool flip_clock_wiseness = false;//set true if flipping such that base is last
        //const vector<int>* activity = nullptr;
        const vector<float>* dvt_id = dvt_info.has_value( ) ? &(arrays->at( "dvt_table_index" )) : nullptr; //zero-based 
        write_ele_files( options, flip_clock_wiseness, &element_pinched_count, dvt_id );
        include_files.push_back( get_filename( options->step( ), options->model_name( ), "ele" ) );

        if(options->auto_config_plasticity( ))
        {
        cout <<"**************************************************************"<<endl;
        cout <<"                  Automatic plasticity parameters             "<<endl;
        cout << "*************************************************************"<< endl;


            if((arrays->contains( "COHESION" )) && (!options->enforce_elastic( )))
            {
                auto& v = arrays->get_array( "COHESION" );
                float avg_cohesion = accumulate( v.begin( ), v.end( ), 0.0f) / v.size( );
                options->set_replace_instruction( "HEADER", "vyieldtolerance", std::to_string( 250.0f));// 0.1 * avg_cohesion ) );
                //FIXME: hard-coded reasonabe value.
                int n_nodes = 1 + (int)( 0.005 * 8 * xyz->size( ) / 3);
                options->set_replace_instruction( "HEADER",  "nyield_gp_number", std::to_string( n_nodes ) );
                options->set_replace_instruction( "HEADER",  "Nsub_increments", std::to_string( 10 ) );
                options->set_replace_instruction( "HEADER",  "Nquickcalculation", std::to_string( 50 ) );
                options->set_replace_instruction( "HEADER",  "Niterations", std::to_string( 10 ) );
                options->set_replace_instruction( "RESULTS", "ele_tot_pl_strain", std::to_string( 1 ) );
            }
        }

        else
        {
            cout << "**************************************************************" << endl;
            cout << "                  Manual plasticity parameters             " << endl;
            cout << "*************************************************************" << endl;

        }
    }

    return write_mii_files( options, include_files );
}
