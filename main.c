//
//  main.c
//  userspacercu
//
//  Created by hari on 14/09/17.
//  Copyright © 2017 hari. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "rcu/rcu.h"

int main(int argc, const char * argv[])
{
	const int nthreads = 2;
	rcu_global_t *rcu_list = malloc(rcu_global_size(nthreads));

	rcu_initialize(rcu_list, nthreads);

	return 0;
}
