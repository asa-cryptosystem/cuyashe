/**
 * cuYASHE
 * Copyright (C) 2015-2016 cuYASHE Authors
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <fstream>
#include <iterator>
#include <iomanip>
#include <cuda_runtime_api.h>
#include <NTL/ZZ.h>
#include <time.h>
#include <unistd.h>
#include <iomanip>
#include "../settings.h"
#include "../yashe/yashe.h"
#include "../aritmetic/polynomial.h"
#include "../logging/logging.h"
#include "../distribution/distribution.h"

#define BILLION  1000000000L
#define MILLION  1000000L
#define N 2

double compute_time_ms(struct timespec start,struct timespec stop){
  return (( stop.tv_sec - start.tv_sec )*BILLION + ( stop.tv_nsec - start.tv_nsec ))/MILLION;
}

 double runEncrypt(Yashe cipher,int d){
  struct timespec start, stop;
  Distribution dist;
  dist = Distribution(UNIFORMLY);

  // Init
  poly_t a,b;
  poly_init(&a);
  dist.generate_sample(&a, 50, d);

  // Exec
  clock_gettime( CLOCK_REALTIME, &start);
  for(int i = 0; i < N;i++){
  	b = cipher.encrypt(a);
    cudaDeviceSynchronize();
  }
  clock_gettime( CLOCK_REALTIME, &stop);
  return compute_time_ms(start,stop)/N;
 }

 double runDecrypt(Yashe cipher, int d){
  struct timespec start, stop;
  Distribution dist;
  dist = Distribution(UNIFORMLY);

  // Init
  poly_t a,b,c;
  poly_init(&a);
  poly_init(&b);
  poly_init(&c);
  dist.generate_sample(&a, 50, d);

  b = cipher.encrypt(a);

  // Exec
  clock_gettime( CLOCK_REALTIME, &start);
  for(int i = 0; i < N;i++){
    c = cipher.decrypt(b);
    cudaDeviceSynchronize();
  }
  clock_gettime( CLOCK_REALTIME, &stop);
  return compute_time_ms(start,stop)/N;
 }

int main(int argc, char* argv[]){
     // Log
    log_init("benchmark.log");
    double diff;

    // Output precision
    cout << fixed;
    cout.precision(2);

    // Init
    int t = 1024;
    ZZ w = NTL::power2_ZZ(32);
    int nq = 127;
    ZZ q = NTL::power2_ZZ(nq) - 1;
    poly_t phi;
    ZZ_pX NTL_Phi;

    for(int d = 512; d < 8192; d*=2){
    	gen_crt_primes(q,d);
	    CUDAFunctions::init(d);
    	poly_init(&phi);
    	poly_set_nth_cyclotomic(&phi,2*d);
	    std::cout << "Degree: " << poly_get_deg(&phi) << std::endl;
    	ZZ_p::init(q);
    	
	    // Init NTL
	    for(int i = 0; i <= poly_get_deg(&phi);i++)
	      NTL::SetCoeff(NTL_Phi,i,conv<ZZ_p>(poly_get_coeff(&phi,i)));
	    ZZ_pE::init(NTL_Phi);

      // Yashe
      Yashe cipher;

      Yashe::nphi = poly_get_deg(&phi);
      Yashe::nq = nq;

      Yashe::t = t;
      Yashe::w = w;
      Yashe::lwq = floor(NTL::log(q)/NTL::log(to_ZZ(w)))+1;

      cipher.generate_keys();

      diff = runEncrypt(cipher, d);
      std::cout << d << " - Encrypt) " << diff << " ms" << std::endl;
      diff = runDecrypt(cipher, d);
      std::cout << d << " - Decrypt) " << diff << " ms" << std::endl;
    }

}