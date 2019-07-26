// goes through every odd number from 3 to the square root
// of num_to_check. If the num_to_check is diveded evenly
// by the dividend, then 0 is returned and num_to_check 
// is not prime. 
// Otherwise the number being checked is prime and 1 is
// returned.

#include "prime-finder.h"

int
is_prime(mpz_t num_to_check)
{
  mpz_t dividend, up_limit, remainder;

  mpz_init_set_ui(dividend, 3);
  mpz_init_set_ui(up_limit, 1);
  mpz_init_set_ui(remainder, 1);

  // mpz_root gets the root and truncates to the int
  // returns 0 if truncated
  if(mpz_root(up_limit, num_to_check, 2) == 0) 
    mpz_add_ui(up_limit, up_limit,1);
  
  // mpz_cmp compares arg1 to arg, returns positive if arg1 > arg2
  // 0 if arg1 == arg2 and negative if arg1 < arg2
  while(mpz_cmp(dividend, up_limit) <= 0)
  {
    mpz_cdiv_r(remainder, num_to_check, dividend);

    // if remainder == 0, reult is fail
    if(mpz_cmp_ui(remainder,0) == 0) return 0;
    
    mpz_add_ui(dividend, dividend, 2);
  }

  return 1;
}
    


int
main(int argc, char **argv)
{
  mpz_t start, stop, remainder, num_to_check;
  int result;

  // converts the char * passed from ./boss to mpz_t 
  // for start and stop
  mpz_init_set_str(start, argv[1], 10);
  mpz_init_set_str(stop, argv[2], 10);
  mpz_init_set(num_to_check, start);
  mpz_init_set(remainder, start);

  if(mpz_cdiv_r_ui(remainder, num_to_check, 2) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    result = is_prime(num_to_check);

    if(result == 1)
      gmp_printf("%Zd is prime\n", num_to_check);

    mpz_add_ui(num_to_check, num_to_check, 2);
  }
}
