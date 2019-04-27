#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>

#include <cstddef>

#include "kernel/yosys.h" 
#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/satgen.h"

USING_YOSYS_NAMESPACE
using namespace std; 
PRIVATE_NAMESPACE_BEGIN

typedef tuple<SigBit, RTLIL::Const> Constraint; 

static RTLIL::Module *module = nullptr; 
static RTLIL::Design *design = nullptr; 

static SigMap sigmap; 

static bool verbosity = false; 

static IdString A = "\\A", B = "\\B", S = "\\S", Y = "\\Y";
static CellTypes basic_ops, not_ops, ff_celltypes; 
static CellTypes and_ops, or_ops, xor_ops; 
static CellTypes inv_ops; 
    
static void setup_types() { 
    // setup basic_ops 
    for (auto type : vector<RTLIL::IdString>({"$reduce_and", "$reduce_or"}))
        basic_ops.setup_type(type, {A}, {Y}, true); 
	for (auto type : vector<RTLIL::IdString>({"$and", "$or", "$logic_and", "$logic_or", "$concat"}))
        basic_ops.setup_type(type, {A, B}, {Y}, true); 

    // setup not_ops
    for (auto type : vector<RTLIL::IdString>({"$not", "$logic_not"}))
        not_ops.setup_type(type, {A}, {Y}, true); 

    // setup and_ops
    for (auto type : vector<RTLIL::IdString>({"$and", "$logic_and"}))
        and_ops.setup_type(type, {A, B}, {Y}, true); 

    // setup or_ops
    for (auto type : vector<RTLIL::IdString>({"$or", "$logic_or"}))
        or_ops.setup_type(type, {A, B}, {Y}, true); 

    // setup xor_ops
    for (auto type : vector<RTLIL::IdString>({"$xor", "$xnor"}))
        xor_ops.setup_type(type, {A, B}, {Y}, true); 

    // setup inv_ops
    inv_ops = not_ops; 
    inv_ops.setup_type("$xnor", {A, B}, {Y}, true); 

    // setup ff_celltypes 
    ff_celltypes.setup_internals_mem();
    ff_celltypes.setup_stdcells_mem();
}

struct SatHelper {
	CellTypes ct;

	ezSatPtr ez;
	SatGen satgen;

	// additional constraints
	vector<pair<string, string>> sets;

	// model variables
	vector<string> shows;
	SigPool show_signal_pool;
	SigSet<RTLIL::Cell*> show_drivers;
	int timeout;
	bool gotTimeout;

	SatHelper() :
		ct(design), satgen(ez.get(), &sigmap) {
		satgen.model_undef = true;

		timeout = 0;
        gotTimeout = false; 
	}
    
	void setup(int timestep = -1) {
		if (verbosity && timestep > 0)
			log ("\nSetting up time step %d:\n", timestep);
		else
			log ("\nSetting up SAT problem:\n");

		RTLIL::SigSpec big_lhs, big_rhs;

		for (auto &s : sets)
		{
			RTLIL::SigSpec lhs, rhs;

			if (!RTLIL::SigSpec::parse_sel(lhs, design, module, s.first))
                lhs = RTLIL::SigSpec(s.first); 
				// log_cmd_error("Failed to parse lhs set expression `%s'.\n", s.first.c_str());
			if (!RTLIL::SigSpec::parse_rhs(lhs, rhs, module, s.second))
				log_cmd_error("Failed to parse rhs set expression `%s'.\n", s.second.c_str());
			show_signal_pool.add(sigmap(lhs));
			show_signal_pool.add(sigmap(rhs));

			if (lhs.size() != rhs.size())
				log_cmd_error("Set expression with different lhs and rhs sizes: %s (%s, %d bits) vs. %s (%s, %d bits)\n",
					s.first.c_str(), log_signal(lhs), lhs.size(), s.second.c_str(), log_signal(rhs), rhs.size());

			if (verbosity) log("Import set-constraint: %s = %s\n", log_signal(lhs), log_signal(rhs));
			big_lhs.remove2(lhs, &big_rhs);
			big_lhs.append(lhs);
			big_rhs.append(rhs);
		}

		if (verbosity) log("Final constraint equation: %s = %s\n", log_signal(big_lhs), log_signal(big_rhs));
		ez->assume(satgen.signals_eq(big_lhs, big_rhs, timestep));

		int import_cell_counter = 0;
		for (auto cell : module->cells())
			if (design->selected(module, cell)) {
				// log("Import cell: %s\n", RTLIL::id2cstr(cell->name));
				if (satgen.importCell(cell, timestep)) {
					for (auto &p : cell->connections())
						if (ct.cell_output(cell->type, p.first))
							show_drivers.insert(sigmap(p.second), cell);
					import_cell_counter++;
				} else 
					log_warning("Failed to import cell %s (type %s) to SAT database.\n", RTLIL::id2cstr(cell->name), RTLIL::id2cstr(cell->type));
		}
		if (verbosity) log("Imported %d cells to SAT database.\n", import_cell_counter);
	}

	bool solve(int a = 0, int b = 0, int c = 0, int d = 0, int e = 0, int f = 0) {
		log_assert(gotTimeout == false);
		ez->setSolverTimeout(timeout);
		bool success = ez->solve(modelExpressions, modelValues, a, b, c, d, e, f);
		if (ez->getSolverTimoutStatus())
			gotTimeout = true;
		return success;
	}

	struct ModelBlockInfo {
		int timestep, offset, width;
		string description;
		bool operator < (const ModelBlockInfo &other) const {
			if (timestep != other.timestep)
				return timestep < other.timestep;
			if (description != other.description)
				return description < other.description;
			if (offset != other.offset)
				return offset < other.offset;
			if (width != other.width)
				return width < other.width;
			return false;
		}
	};

	vector<int> modelExpressions;
	vector<bool> modelValues;
	set<ModelBlockInfo> modelInfo;

	void maximize_undefs() {
		vector<bool> backupValues;

		while (1) {
			vector<int> must_undef, maybe_undef;

			for (size_t i = 0; i < modelExpressions.size()/2; i++)
				if (modelValues.at(modelExpressions.size()/2 + i))
					must_undef.push_back(modelExpressions.at(modelExpressions.size()/2 + i));
				else
					maybe_undef.push_back(modelExpressions.at(modelExpressions.size()/2 + i));

			backupValues.swap(modelValues);
			if (!solve(ez->expression(ezSAT::OpAnd, must_undef), ez->expression(ezSAT::OpOr, maybe_undef)))
				break;
		}

		backupValues.swap(modelValues);
	}

	void generate_model() {
		RTLIL::SigSpec modelSig;
		modelExpressions.clear();
		modelInfo.clear();

		// Add "show" signals or alternatively the leaves on the input cone on all set and prove signals

		if (shows.size() == 0) {
			SigPool queued_signals, handled_signals, final_signals;
			queued_signals = show_signal_pool;
			while (queued_signals.size() > 0) {
				RTLIL::SigSpec sig = queued_signals.export_one();
				queued_signals.del(sig);
				handled_signals.add(sig);
				set<RTLIL::Cell*> drivers = show_drivers.find(sig);
				if (drivers.size() == 0) {
					final_signals.add(sig);
				} else {
					for (auto &d : drivers)
					for (auto &p : d->connections()) {
						if (d->type == "$dff" && p.first == "\\CLK")
							continue;
						if (d->type.substr(0, 6) == "$_DFF_" && p.first == "\\C")
							continue;
						queued_signals.add(handled_signals.remove(sigmap(p.second)));
					}
				}
			}
			modelSig = final_signals.export_all();

			// additionally add all set and prove signals directly
			// (it improves user confidence if we write the constraints back ;-)
			modelSig.append(show_signal_pool.export_all());
		}
		else {
			for (auto &s : shows) {
				RTLIL::SigSpec sig;
				if (!RTLIL::SigSpec::parse_sel(sig, design, module, s))
					log_cmd_error("Failed to parse show expression `%s'.\n", s.c_str());
				if (verbosity) log("Import show expression: %s\n", log_signal(sig));
				modelSig.append(sig);
			}
		}

		modelSig.sort_and_unify();
		// log("Model signals: %s\n", log_signal(modelSig));

		vector<int> modelUndefExpressions;

		for (auto &c : modelSig.chunks())
			if (c.wire != NULL) {
				ModelBlockInfo info;
				RTLIL::SigSpec chunksig = c;
				info.width = chunksig.size();
				info.description = log_signal(chunksig);

				info.timestep = -1;
				info.offset = modelExpressions.size();
				modelInfo.insert(info);

				vector<int> vec = satgen.importSigSpec(chunksig, -1);
				modelExpressions.insert(modelExpressions.end(), vec.begin(), vec.end());

				std::vector<int> undef_vec = satgen.importUndefSigSpec(chunksig, -1);
				modelUndefExpressions.insert(modelUndefExpressions.end(), undef_vec.begin(), undef_vec.end());
			}

		// Add initial state signals as collected by satgen
		//
		modelSig = satgen.initial_state.export_all();
		for (auto &c : modelSig.chunks())
			if (c.wire != NULL) {
				ModelBlockInfo info;
				RTLIL::SigSpec chunksig = c;

				info.timestep = 0;
				info.offset = modelExpressions.size();
				info.width = chunksig.size();
				info.description = log_signal(chunksig);
				modelInfo.insert(info);

				vector<int> vec = satgen.importSigSpec(chunksig, 1);
				modelExpressions.insert(modelExpressions.end(), vec.begin(), vec.end());
				vector<int> undef_vec = satgen.importUndefSigSpec(chunksig, 1);
				modelUndefExpressions.insert(modelUndefExpressions.end(), undef_vec.begin(), undef_vec.end());
			}

		modelExpressions.insert(modelExpressions.end(), modelUndefExpressions.begin(), modelUndefExpressions.end());
	}

	void print_model(vector<tuple<string, RTLIL::Const> >& input_const) {
		int maxModelName = 10;
		int maxModelWidth = 10;

		for (auto &info : modelInfo) {
			maxModelName = max(maxModelName, int(info.description.size()));
			maxModelWidth = max(maxModelWidth, info.width);
		}

        if (verbosity) log("\n");

		int last_timestep = -2;
		for (auto &info : modelInfo) {
			RTLIL::Const value;
			bool found_undef = false;

			for (int i = 0; i < info.width; i++) {
				value.bits.push_back(modelValues.at(info.offset+i) ? RTLIL::State::S1 : RTLIL::State::S0);
				if (modelValues.at(modelExpressions.size()/2 + info.offset + i))
					value.bits.back() = RTLIL::State::Sx, found_undef = true;
			}

            if (value.is_fully_undef()) continue; 

			if (info.timestep != last_timestep) {
                if (verbosity) { 
				    const char *hline = "---------------------------------------------------------------------------------------------------"
				    		    "---------------------------------------------------------------------------------------------------"
				    		    "---------------------------------------------------------------------------------------------------";
				    if (last_timestep == -2) {
				    	log("  ");
				    	log("%-*s %11s %9s %*s\n", maxModelName+5, "Signal Name", "Dec", "Hex", maxModelWidth+3, "Bin");
				    }
				    log("  ");
				    log("%*.*s %11.11s %9.9s %*.*s\n", maxModelName+5, maxModelName+5,
				    		hline, hline, hline, maxModelWidth+3, maxModelWidth+3, hline);
                }
				last_timestep = info.timestep;
			}

            input_const.push_back(make_tuple(info.description, value)); 
            if (verbosity) { 
			    log("  ");

			    if (info.width <= 32 && !found_undef)
			    	log("%-*s %11d %9x %*s\n", maxModelName+5, info.description.c_str(), value.as_int(), value.as_int(), maxModelWidth+3, value.as_string().c_str());
			    else
			    	log("%-*s %11s %9s %*s\n", maxModelName+5, info.description.c_str(), "--", "--", maxModelWidth+3, value.as_string().c_str());
            }
		}

		if (verbosity && last_timestep == -2)
			log("  no model variables selected for display.\n");
	}

} *sat_solver; 

struct Path { 
    vector<tuple<SigBit, bool> > path; 
    dict<SigBit, Constraint> const0, const1; 
    vector<tuple<string, RTLIL::Const> > iconst0, iconst1; 

    Path() {} 
    Path(SigBit bit, SigBit from, Cell *via, bool is_rising) { this->add(bit, from, via, is_rising); } 

    bool empty() { return path.empty(); } 

    Path copy() { 
        Path ret; 
        ret.path = path; 
        ret.const0= const0; 
        ret.const1= const1; 
        ret.iconst0 = iconst0; 
        ret.iconst1 = iconst1; 

        return ret; 
    }

    Path add(SigBit bit, SigBit from, Cell *via, bool is_rising, bool is_inversed=false) { 
        path.insert(path.begin(), make_tuple(bit, is_rising)); 
        extract(bit, from, via, is_rising, is_inversed); 

        return *this; 
    }

    void extract(SigBit bit, SigBit from, Cell *via, bool is_rising, bool is_inversed) { 
        // TODO: handle multiple input port feeding 
        RTLIL::IdString on_port; 
        for (auto &conn : via->connections())
            for (auto bit : sigmap(conn.second)) 
                if (via->input(conn.first) && bit==from) { 
                    on_port = conn.first; 
                    break; 
                }

        if (via->type=="$mux" && sigmap(via->getPort(S))!=from) {  
            auto &ctrl = via->getPort(S); 
            auto sbit = (on_port!=A) ? RTLIL::Const(RTLIL::State::S1) : \
                RTLIL::Const(RTLIL::State::S0); 
            const1[bit] = Constraint(ctrl, sbit); 
        } else if (and_ops.cell_known(via->type)) { 
            auto &side_port = (on_port==A) ? via->getPort(B) : via->getPort(A); 
            const1[bit] = Constraint(side_port, RTLIL::Const(RTLIL::State::S1)); 
        } else if (or_ops.cell_known(via->type)) { 
            auto &side_port = (on_port==A) ? via->getPort(B) : via->getPort(A); 
            const1[bit] = Constraint(side_port, RTLIL::Const(RTLIL::State::S0)); 
        } else if (xor_ops.cell_known(via->type)) {
            auto &side_port = (on_port==A) ? via->getPort(B) : via->getPort(A); 
            auto sbit = (is_inversed^inv_ops.cell_known(via->type)) ? \
                RTLIL::Const(RTLIL::State::S0) : RTLIL::Const(RTLIL::State::S1); 
            const0[bit] = Constraint(side_port, sbit); 
            const1[bit] = Constraint(side_port, sbit); 
        } else { 
            const0[bit] = Constraint(bit, RTLIL::Const(!is_rising, 1)); 
            const1[bit] = Constraint(bit, RTLIL::Const(is_rising, 1)); 
        }
        // TODO: else if (via->type=="$pmux")
    }

    bool sat(bool max_undef=true) { 
        // string cmd = "sat -seq 2 -ignore_unknown_cells -max_undef -show-inputs"; 
        string select_cmd = "select "; 
        select_cmd = select_cmd + strip_(log_signal(get<0>(path[path.size()-1])), true) + " %ci2 %cie*";   
        run_pass(select_cmd); 

        return sat_(const0, iconst0, max_undef) && sat_(const1, iconst1, max_undef); 
    }

    void write_file(ostream &os) { 
        os << "$transition {\n"; 
        size_t i = 0; 
        for (auto bit : path) {  
            auto b = get<0>(bit); 
            os << "\t\"" << strip_(string(log_signal(b))) << "\" "; 
            os << ((get<1>(bit))?'^':'v') << " ;"; 
            os << " // ("; 
            if (i==0 && iconst0.size()>0) { 
                for (size_t j=0; j<iconst0.size(); j++) { 
                    os << strip_(get<0>(iconst0[j])) << '=' << get<1>(iconst0[j]).as_string(); 
                    if (j<iconst0.size()-1) os << '&'; 
                }
            } else if (const0.count(b)) { 
                os << log_signal(get<0>(const0.at(b))); 
                os << '=' << get<1>(const0.at(b)).as_string(); 
            }
            os << ')'; 

            os << " ("; 
            if (i==0 && iconst1.size()>0) { 
                for (size_t j=0; j<iconst1.size(); j++) { 
                    os << strip_(get<0>(iconst1[j])) << '=' << get<1>(iconst1[j]).as_string(); 
                    if (j<iconst1.size()-1) os << '&'; 
                }
            } else if (const1.count(b)) { 
                os << log_signal(get<0>(const1.at(b))); 
                os << '=' << get<1>(const1.at(b)).as_string(); 
            }
            os << ')' << endl;
            i++; 
        }
        
        os << "}\n\n"; 
    }

private: 
    bool sat_(const dict<SigBit, Constraint>& constraints, vector<tuple<string, RTLIL::Const> >& input_const, bool max_undef) { 
        sat_solver = new SatHelper; 
		// for (auto &it : module->wires_)
		// 	if (it.second->port_input)
		// 		sat_solver->shows.push_back(it.second->name.str());
        
        for (auto &c : constraints) { 
            sat_solver->sets.push_back(pair<string, string>(\
                strip_(string(log_signal(get<0>(c.second)))),\
                (get<1>(c.second).as_bool())?"1":"0")); 
        }
        sat_solver->setup(); 
        sat_solver->generate_model(); 
        if (verbosity) { 
		    log("\nSolving problem with %d variables and %d clauses..\n",
		        sat_solver->ez->numCnfVariables(), sat_solver->ez->numCnfClauses());
		    log_flush();
        }
        if (sat_solver->solve()) { 
            if (verbosity) { 
			    log("SAT solving finished - model found:\n"); 
			    log("SAT model found. maximizing number of undefs.\n");
            } 
			if (max_undef) sat_solver->maximize_undefs();
            sat_solver->print_model(input_const); 

            delete sat_solver; 
            return true; 
        } else if (verbosity) 
			log("SAT solving finished - no model found.\n");

        delete sat_solver; 
        return false; 
    }

    string strip_(string str, bool strip_idx=false) { 
        if (!strip_idx) 
            str.erase(remove(str.begin(), str.end(), ' '), str.end()); 
        else { 
            string::size_type pos = str.find(' '); 
            str = (pos==string::npos) ? str : str.substr(0, pos); 
        }
        return str; 
}

}; 

struct CDFGWorker { 
    dict<SigBit, tuple<int, pool<SigBit>, Cell*>> bits;
    dict<SigBit, dict<SigBit, Cell*>> bit2bits;
    dict<SigBit, tuple<SigBit, Cell*>> bit2ff;

    int max_n_path; 
    vector<Path> paths; 

    int ut, pd; 

    int maxlvl;
    SigBit maxbit;
    pool<SigBit> busy;

    CDFGWorker(int max_paths) : max_n_path(max_paths){ 
        setup_types(); 
        ut = pd = 0; 

        for (auto wire : module->selected_wires())
            for (auto bit : sigmap(wire))
                bits[bit] = tuple<int, pool<SigBit>, Cell*>(-1, pool<SigBit>(), nullptr);

        for (auto cell : module->selected_cells()) { 
            pool<SigBit> src_bits, dst_bits;

            for (auto &conn : cell->connections())
                for (auto bit : sigmap(conn.second)) {
                    if (cell->input(conn.first))
                        src_bits.insert(bit);
                    if (cell->output(conn.first))
                        dst_bits.insert(bit);
                }

            if (ff_celltypes.cell_known(cell->type)) {
                for (auto s : src_bits)
                    for (auto d : dst_bits) {
                        bit2ff[s] = tuple<SigBit, Cell*>(d, cell);
                        break;
                    }
                continue;
            }

            for (auto s : src_bits)
                for (auto d : dst_bits)
                    bit2bits[s][d] = cell;
        }

        maxlvl = -1;
        maxbit = State::Sx;
    }

    void runner(SigBit bit, int level, Cell *via, SigBit from=State::Sx) {
        if (busy.count(bit) > 0) {
            log_warning("Detected loop at %s in %s\n", log_signal(bit), log_id(module));
            return;
        }

        auto &bitinfo = bits.at(bit);
        if (from!=State::Sx)
            get<1>(bitinfo).insert(from);

        if (get<0>(bitinfo) >= level)
            return;

        busy.insert(bit);
        get<0>(bitinfo) = level;
        get<2>(bitinfo) = via;

        if (level > maxlvl) {
            maxlvl = level;
            maxbit = bit;
        }

        if (bit2bits.count(bit)) {
            for (auto &it : bit2bits.at(bit))
                runner(it.first, level+1, it.second, bit);
        }

        busy.erase(bit);
    }

    void backtrace(SigBit bit, Path p, bool is_rising) {
        if (paths.size()>=(size_t)max_n_path) return; 

        auto &bitinfo = bits.at(bit);
        if (get<2>(bitinfo)) {
            if (!p.sat(false)) return; 
            multimap<int, SigBit, greater<int> > mmap; 
            for (auto &b : get<1>(bitinfo))
                mmap.insert({get<0>(bits.at(b)), b}); 
            for (auto &it : mmap) { 
                if (paths.size()>=(size_t)max_n_path) return; 
                Cell *via = get<2>(bitinfo); 
                if (basic_ops.cell_known(via->type))
                    backtrace(it.second, p.copy().add(bit, it.second, via, is_rising), is_rising);
                else if (not_ops.cell_known(via->type))
                    backtrace(it.second, p.copy().add(bit, it.second, via, is_rising, true), !is_rising);
                else if (via->type=="$mux" && sigmap(via->getPort(S))!=it.second)  
                    backtrace(it.second, p.copy().add(bit, it.second, via, is_rising), is_rising);
                else { 
                    backtrace(it.second, p.copy().add(bit, it.second, via, is_rising), is_rising);
                    backtrace(it.second, p.copy().add(bit, it.second, via, is_rising, true), !is_rising);
                }
            } 
        } else {
            p.path.insert(p.path.begin(), make_tuple(bit, is_rising)); 
            p.const0[bit] = Constraint(bit, RTLIL::Const(!is_rising, 1)); 
            p.const1[bit] = Constraint(bit, RTLIL::Const(is_rising, 1)); 
            if (p.sat()) { 
                paths.push_back(p); 
                pd+=1; 
            } else { ut+=1; }
        }
    }

    void run() { 
        for (auto &it : bits)
            if (get<0>(it.second) < 0)
                runner(it.first, 0, nullptr);

        if (maxlvl >= 0) { 
            if (bit2ff.count(maxbit)) {
                backtrace(maxbit, Path(get<0>(bit2ff.at(maxbit)), maxbit, get<1>(bit2ff.at(maxbit)), true), true);
                backtrace(maxbit, Path(get<0>(bit2ff.at(maxbit)), maxbit, get<1>(bit2ff.at(maxbit)), false), false);
            } else { 
                backtrace(maxbit, Path(), true);
                backtrace(maxbit, Path(), false);
            }
        }
    }

}; 

struct CDFGExtractPass : public Pass {
    CDFGExtractPass() : Pass("cdfg", "RTL CDFG Extraction") { }

    void help() YS_OVERRIDE { 
        //TODO 
        return;  
    }

    void execute(vector<string> args, RTLIL::Design *_design) YS_OVERRIDE {
        int max_paths = 1; 

        design = _design; 
        log_header(design, "Executing CDFG pass (CDFG Extraction).\n");

        if (design->selected_active_module.empty())  
            module = design->top_module(); 
        else if (design->module(design->selected_active_module) != nullptr)  
            module = design->module(design->selected_active_module);

        if (module->has_memories_warn() || module->has_processes_warn())  
            log_cmd_error("unsupported module \"%s\"\n", module->name.c_str()); 

        size_t argidx; 
        for (argidx=1; argidx<args.size(); argidx++) { 
            if (args[argidx]=="-max_paths" && argidx+1<args.size()) { 
                max_paths = atoi(args[++argidx].c_str()); 
                continue; 
            } 
            else if (args[argidx]=="-v") { 
                verbosity = true; 
                continue; 
            } 
            break; 
        } 
        extra_args(args, argidx, design); 

        sigmap = SigMap(module); 

        CDFGWorker worker(max_paths); 
        worker.run(); 
        log("Coverage = %d / %d\n", worker.pd, worker.ut+worker.pd); 

        ofstream ofs; ofs.open(string(log_id(module->name)) + ".path"); 
        for (auto &p : worker.paths) 
            p.write_file(ofs); 

        ofs.close(); 

        run_pass("select *"); 
    }

} CDFGExtractPass;

PRIVATE_NAMESPACE_END
