/*
 * Copyright © 2016 George Petropoulos.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */
package eu.point.bootstrapping.impl.icn;

import eu.point.bootstrapping.impl.BootstrappingServiceImpl;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * The Combinations class.
 * It includes all the required methods to make all possible combinations with the edge
 * connectors of a particular switch.
 *
 * @author Marievi Xezonaki
 * @version cycle-2
 */
public class Combinations {

    public static String output = "";
    private static final Logger LOG = LoggerFactory.getLogger(Combinations.class);

    /**
     * The method which makes all possible K-element combinations with the elements of a given list.
     *
     * @param elements A list of elements to be combined.
     * @param K The number of each combination's elements.
     */
    public static void combination(List<String> elements, int K){

        int N = elements.size();
        if(K > N){
            System.out.println("Invalid input, K > N");
            return;
        }

        int num = c(N,K);

        int combination[] = new int[K];
        int r = 0;
        int index = 0;

        output = "";
        while(r >= 0){
            if(index <= (N + (r - K))){
                combination[r] = index;

                // if we are at the last position print and increase the index
                if(r == K-1){
                    //do something with the combination e.g. add to list or print
                    String out = allCombinations(combination, elements);
                    output += out + "#";
            //        System.out.println("Called for " + N + "elements and " + K + " el/comb " + out);
                    index++;
                }
                else{
                    // select index for next position
                    index = combination[r]+1;
                    r++;
                }
            }
            else{
                r--;
                if(r > 0)
                    index = combination[r]+1;
                else
                    index = combination[0]+1;
            }
        }
    }

    public static int c(int n, int r){
        int nf=fact(n);
        int rf=fact(r);
        int nrf=fact(n-r);
        int npr=nf/nrf;
        int ncr=npr/rf;

  //      System.out.println("C("+n+","+r+") = "+ ncr);

        return ncr;
    }

    /**
     * The method which computes the factorial of a number.
     *
     * @param n An integer.
     * @return The factorial of an integer.
     */
    public static int fact(int n)
    {
        if(n == 0)
            return 1;
        else
            return n * fact(n-1);
    }

    /**
     * The method which concatenates all members of a combination into a single string.
     *
     * @param combination One combination of the list.
     * @param elements A list of elements.
     * @return A string with all the members of the combination concatenated.
     */
    public static String allCombinations(int[] combination, List<String> elements){

        String out = "";
        for(int z = 0 ; z < combination.length;z++){
            out += elements.get(combination[z]);
            out += ",";
        }
        return out;
    }

    public static String getOutput(){
        return output;
    }
}


