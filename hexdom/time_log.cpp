/*
 *  Copyright (c) 2000-2022 Inria
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  * Neither the name of the ALICE Project-Team nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Contact: Bruno Levy
 *
 *     https://www.inria.fr/fr/bruno-levy
 *
 *     Inria,
 *     Domaine de Voluceau,
 *     78150 Le Chesnay - Rocquencourt
 *     FRANCE
 *
 */

#include <exploragram/hexdom/time_log.h>

LogTime logt ;

bool LogTime::is_start_section(unsigned int i)   { return check[i].right != i + 1; }
bool LogTime::is_end_section(unsigned int i)             { return check[i].n == "end section"; }
bool LogTime::is_final(unsigned int i)                   { return i + 1 == check.size(); }
double LogTime::time(unsigned int i)                             { return double(check[check[i].right].t - check[i].t) / double(CLOCKS_PER_SEC); }


unsigned int LogTime::dec(unsigned int i){
    if (i == (unsigned int)(-1))
        i = (unsigned int)(check.size() - 1);
    unsigned int res = 0;
    i = check[i].up;
    while (i != (unsigned int)(-1)) {
        res++;
        i = check[i].up;
    }
    return res;
}

unsigned int LogTime::lastdec(){
    if (!check.empty())
        return dec((unsigned int)(check.size() - 1));
    return 0;
}

void LogTime::debug(){
    GEO::Logger::out("HexDom")  <<  std::endl;
    GEO::Logger::out("HexDom")  << "--------------BEGIN DEBUG-------------------" <<  std::endl;
    for (size_t i = 0; i < check.size(); i++) {
        std::cerr << std::string((unsigned int)(4 * dec((unsigned int)(i))), ' ') << i << "  r = " << check[i].right << " u = " << check[i].up
                  << "\tstart" << check[i].t << "\tname" << check[i].n << std::endl;
    }
    GEO::Logger::out("HexDom")  <<  std::endl;
    GEO::Logger::out("HexDom")  <<"-------------- END  DEBUG-------------------" <<  std::endl;
}

std::string LogTime::cur_stack(){
    std::string res;
    std::vector<unsigned int> stack;
    {
        unsigned int i = (unsigned int)(check.size() - 1);
        while (i != (unsigned int)(-1)) { stack.push_back(i); i = check[i].up; }
    }
    for (int i = int(stack.size()) - 1; i >= 0; i--) {
        res.append(check[stack[size_t(i)]].n);
        if (i > 0) res.append(" ==> ");
    }
    return res;
}

void LogTime::report(std::ostream &out, unsigned int timing_depth){
    if (check.empty()) return;
    if (check.back().n != "the end") { add_step("the end"); }

    if (timing_depth != (unsigned int)(-1)) {
        out << "\n***********************************************************" << std::endl;
        out << "                  TIMING SUMMARY " << std::endl;
        for (unsigned int i = 0; i < check.size() - 1; i++){
            if (dec(i)>timing_depth) continue;
            if (is_start_section(i))
                out << std::string(4 * dec(i), ' ') << time(i) << "\t====>  " << check[i].n << std::endl;
            else if (!is_end_section(i)){
                if (check[i].n != "begin section"){
                    out << std::string(4 * dec(i), ' ') << time(i) << "\t" << check[i].n << std::endl;
                }
            }
        }

        out << double(check[check.size() - 1].t - check[0].t) / double(CLOCKS_PER_SEC) << "\tTOTAL" << std::endl;
    }
    out << "\n***********************************************************" << std::endl;
    out << "                  OUPUT VALUES" << std::endl;
    for (size_t i = 0; i < out_values.size(); i++)
        out << out_values[i].second << " \t" << out_values[i].first << std::endl;

}



void LogTime::report_py(std::ostream &out, unsigned int timing_depth){
    if (check.empty()) return;
    if (check.back().n != "the end") { add_step("the end"); }

    out << "{ \"charlie\": 2";
    // bool first = true;
    if (timing_depth != (unsigned int)(-1)) {
        for (unsigned int i = 0; i < check.size() - 1; i++){
            if (dec(i)>timing_depth) continue;
            if (is_start_section(i)) out <<",\"TIME_" << check[i].n  <<"\" :  "<< time(i);
            else if (!is_end_section(i)){
                if (check[i].n != "begin section"){
                    out << ",\"TIME_" << check[i].n << "\":  " << time(i)  ;
                }
            }
        }

        //out << "\ttest[\'TIME_TOTAL\'] = " << double(check[check.size() - 1].t - check[0].t) / double(CLOCKS_PER_SEC) << std::endl;
    }
    for (size_t i = 0; i < out_values.size(); i++)
        out << ", \"" << out_values[i].first << "\": " << out_values[i].second ;
    for (size_t i = 0; i < out_strings.size(); i++)
        out << ",\"" << out_strings[i].first << "\": \"" << out_strings[i].second <<"\"";
    out << "}" << std::endl;

}





// construct API


void LogTime::add_value(std::string str, double val) {
    GEO::Logger::out("HexDom")  << "log new value : " << str<< post_fix << " = " << val <<  std::endl;
    out_values.push_back(std::pair<std::string, double>(str+ post_fix, val));
}
void LogTime::add_string(std::string str , std::string val) {
    GEO::Logger::out("HexDom")  << "log new string  : " << str << post_fix << " = " << val <<  std::endl;
    out_strings.push_back(std::pair<std::string, std::string>(str+ post_fix, val));
}

void LogTime::add_step(const std::string& name){
    if (check.empty()){
        CheckPoint c(name + post_fix, (unsigned int)(-1));
        check.push_back(c);
    }
    else {
        CheckPoint c(name + post_fix, check.back().up);
        check.back().right = (unsigned int)(check.size());
        check.push_back(c);
    }
    const char *symbol = "#>=_-~..............";
    GEO::Logger::out("HexDom")  << std::endl << std::string(4 * lastdec(), ' ') << std::string(80 - 4 * lastdec(), symbol[dec()]) <<  std::endl;
    GEO::Logger::out("HexDom")  << std::string(4 * lastdec() + 4, ' ') << cur_stack() << std::endl <<  std::endl;
}

void LogTime::start_section(const std::string& secname, const std::string& name ){
    add_step(secname + post_fix);
    CheckPoint c(name + post_fix, (unsigned int)(check.size()) - 1);
    check.push_back(c);
}
void LogTime::end_section(){
    unsigned int u = check.back().up;
    check[u].right = (unsigned int)(check.size());
    check.back().right = (unsigned int)(check.size());
    CheckPoint c("end section", check[u].up);
    check.push_back(c);
}


// output
void LogTime::show(){
    report(std::cerr);
}

void LogTime::drop_file(std::string filename, bool append, unsigned int timing_depth){
    std::ofstream f;
    if (append)
        f.open(filename.c_str(), std::fstream::app);
    else
        f.open(filename.c_str());
    report_py(f, timing_depth);
    f.close();
}
