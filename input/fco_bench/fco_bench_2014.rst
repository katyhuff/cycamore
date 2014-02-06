Fuel Cycle Transition Scenario With Cyclus Problem Definition
=============================================================



Scenario
--------

- Start Date : January 2014
- End date : When 100% transition to SFRs is complete (<100 years)
- Total Power Demand is Constant (90GWe)
- Total Installed Capacity is Constant (100GWe)
- Initial State :

  - 0 tHM UNF available for reprocessing
  - Initial deployment : 100 LWRs
  - 400,000 tHM depleted U inventory
  - Unlimited natural U inventory


Facilities
----------

LWRs
....

- 1000 MWe per reactor
- 0.90 Capacity Factor
- 3 batches per core
- Deployment : initial 2014 deployment only (100 LWRs)
- Decommissioning : decommission 1 LWR per 3 SFRs built
- Burnup = 50 MWd/kgIHM
- Cycle length = 1.5 calendar years
- Licensing time = 2 years
- Construction time = 4 years 
- Input:
    
  - **fresh_lwr_fuel**

- Output:

  - power
  - **spent_lwr_fuel**
  
    - composition (wt%)
    - U = 93.44
    - Pu = 1.20
    - MA = 0.10 
    - FP = 5.26

SFRs
....

- 333.3 MWe per reactor (1000/3)
- 0.90 Capacity Factor
- Deployment : deploy 3 when sufficient SFR fuel is available (83515 tons LWR UNF)
- Decomissioning (after 60 years)
- 3.3 batches per core?
- Burnup = 73 MWd/kgIHM
- Cycle length = 1.3 calendar years
- Licensing time = 2 years
- Construction time = 4 years 
- Input:
    
  - **fresh_sfr_fuel**

- Output:

  - **power**
  - **spent_sfr_fuel**
  
    - composition (wt%)
    - U = 78.30
    - Pu = 13.56
    - MA = 0.48 
    - FP = 7.66


LWR UNF Cooling Storage
...................

- LWR fuel must be cooled for 4 years before reprocessing
- Input: 
  
  - **lwr_unf**

- Output: 

  - **lwr_unf_cool**

SFR UNF Cooling Storage
...................

- SFR fuel must be cooled for 1 year before reprocessing
- Input: 
  
  - **sfr_unf**

- Output: 

  - **sfr_unf_cool**


LWR UNF Reprocessing
.....................

- Start Date : 2025
- 2000 tHM/year Capacity
- There are no reprocessing losses
- Input: 
  
  - **lwr_unf_cool**

- Output: 

  - **rep_lwr_u**
  - **rep_lwr_tru**


SFR UNF Reprocessing
.....................

- Start Date : when needed
- Unlimited Capacity
- There are no reprocessing losses
- Input:
  
  - **sfr_unf_cool**

- Output: 

  - **rep_sfr_u**
  - **rep_sfr_tru**


Enrichment
..........

- Natural U enrichment = 0.711 wt%
- Depleted U enrichment =  0.25 wt%
- ''Enrichment Time'' for LWR fuel = 1 year
- Input : 

  - **nat_u**
    
- Output : 

  - **leu**
  - **dep_u**

LWR Fuel Fabrication
....................

- Fabrication time = 1 year
- Fissionable material source = LEU
- Input:

  - **leu** (4.3% 235U)

- Output:

  - **fresh_lwr_fuel**


SFR Fuel Fabrication
....................

- Fabrication time = 1 year
- Input (by preference):

  - fissile_mat

    #. **rep_sfr_tru**
    #. **rep_lwr_tru**
  
  - fertile_mat

    #. **rep_sfr_u**
    #. **rep_lwr_u**
    #. **dep_u**
    #. **nat_u**

- Output : 

  - **fresh_sfr_fuel**
  - Fresh Fuel Composition (wt%):
  - U = 86.13
  - Pu = 13.42
  - MA = 0.45



Desired Outputs
---------------

- Name the year during which the transition becomes complete
- LWR and SFR deployed capacity profile (should demonstrate that there were no potential generating shortages)
- Separated surplus PU or TRU profile
- LWR used fuel reprocessing rate (t/yr)
- SFR used fuel reprocessing rate (t/yr)
- LWR used fuel mass in storage (t)
- SFR used fuel mass in storage (t)
