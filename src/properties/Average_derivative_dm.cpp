/*
 
 Copyright (C) 2016 Lucas K. Wagner
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
 */


#include "Average_derivative_dm.h"
#include <cmath>
#include "Properties_point.h"
//######################################################################


void Average_derivative_dm::evaluate(Wavefunction_data * wfdata, Wavefunction * wf,
                       System * sys, Sample_point * sample, Properties_point & pt,
                       Average_return & avg) { 
  avg.type="derivative_dm";

  Average_return dmavg;
  dm_eval.evaluate(wfdata,wf,sys,sample,dmavg);
  ndm_elements=dmavg.vals.GetDim(0)-nmo;
  
  Parm_deriv_return deriv;
  deriv.need_hessian=0;
  if(!wf->getParmDeriv(wfdata, sample,deriv)) {
    error("Wave function needs to support parmderivs for Average_derivative_dm.");
  }
  
  //The serialization should be:
  //[0:nparms] Average values of derivatives
  //[nparms:2*nparms] Correlation of parameter derivatives with local energy
  //[2*nparms:2*nparms+nmo] Orbital normalization
  //[2*nparms+nmo:2*nparms+nmo+ndm_elements] DM averages
  //[2*nparms+nmo+ndm_elements : 2*nparms+nmo+ndm_elements+ndm_elements*nparms]

  avg.vals.Resize(2*nparms+nmo+ndm_elements+ndm_elements*nparms);
  int count=0;
  for(int p=0; p < nparms; p++) {
    avg.vals(count++)=deriv.gradient(p);
  }
  for(int p=0; p < nparms; p++) {
    avg.vals(count++)=deriv.gradient(p)*pt.energy(0);
  }
  
  for(int d=0; d< nmo+ndm_elements; d++) {
    avg.vals(count++)=dmavg.vals(d);
  }
  
  for(int p=0; p < nparms; p++) {
    for(int d=0;d < ndm_elements; d++) {
      avg.vals(count++)=deriv.gradient(p)*dmavg.vals(nmo+d);
    }
  }
  
}
//-----------------------------------------------------------------------------
void Average_derivative_dm::read(System * sys, Wavefunction_data * wfdata, vector
                   <string> & words) { 
  unsigned int pos=0;
  vector <string> avgsec;
  if(!readsection(words, pos=0,avgsec,"AVERAGE"))
    error("Need AVERAGE section in AVERAGE_DERIVATIVE_DM");
  dm_eval.read(sys,wfdata,avgsec);
  nparms=wfdata->nparms();
  nmo=dm_eval.get_nmo();
  ndm_elements=dm_eval.get_val_size()-nmo;

  
}
//-----------------------------------------------------------------------------
void Average_derivative_dm::write_init(string & indent, ostream & os) { 
  os << indent << "AVERAGE_DERIVATIVE_DM" << endl;
  os << indent << "AVERAGE { ";
  dm_eval.write_init(indent,os);
  os << indent << "}" << endl;
  os << indent << "NMO " << nmo << endl;
  os << indent << "NDM_ELEMENTS " << ndm_elements << endl;
  os << indent << "NPARMS " << nparms << endl;
}
//-----------------------------------------------------------------------------
void Average_derivative_dm::read(vector <string> & words) { 
  vector<string> avgsec;
  unsigned int pos=0;
  if(!readsection(words, pos=0,avgsec,"AVERAGE"))
    error("Need AVERAGE section in ENMOMENT");
  dm_eval.read(avgsec);
  readvalue(words,pos=0,nmo,"NMO");
  readvalue(words,pos=0,ndm_elements,"NDM_ELEMENTS");
  readvalue(words,pos=0,nparms,"NPARMS");
}
//-----------------------------------------------------------------------------
void Average_derivative_dm::write_summary(Average_return &avg ,Average_return & err,
    ostream & os) {
  
  int count=0;
  os << "Wave function derivatives" << endl;
  for(int p=0; p < nparms; p++) {
    os << avg.vals(count) << " +/- " << err.vals(count) << endl;
    count++;
  }
  
  os << "Derivative of the energy" << endl;
  for(int p=0; p < nparms; p++) {
    os << avg.vals(count) << " +/- " << err.vals(count) << endl;
    count++;
  }

  Average_return tmpavg,tmperr;
  tmpavg.vals.Resize(nmo+ndm_elements);
  tmperr.vals.Resize(nmo+ndm_elements);
  os << "Average density matrix values" << endl;
  for(int d=0; d< nmo+ndm_elements; d++) {
    tmpavg.vals(d)=avg.vals(count);
    tmperr.vals(d)=err.vals(count);
    count++;
  }
  dm_eval.write_summary(tmpavg,tmperr,os);
  
  

  os << "Density matrix changes correlated with parameter derivatives" << endl;
  for(int p=0; p < nparms; p++) {
    for(int d=0;d < ndm_elements; d++) {
      tmpavg.vals(nmo+d)=avg.vals(count);
      tmperr.vals(nmo+d)=err.vals(count);
      count++;
    }
    dm_eval.write_summary(tmpavg,tmperr,os);
  }
}

//-----------------------------------------------------------------------------
void Average_derivative_dm::jsonOutput(Average_return & avg,Average_return & err, ostream & os) {
  int counta=0;
  int counte=0;
  os <<"\""<< avg.type << "\":{" << endl;
  os << "\"nmo\":" << nmo <<","<< endl;
  os << "\"dpwf\":{" << endl;
  os << "\"vals\":[";
  for(int p=0; p < nparms-1; p++) os << avg.vals(counta++) << ",";
  os << avg.vals(counta++) << "],";
  os << "\"err\":[";
  for(int p=0; p < nparms-1; p++) os << err.vals(counte++) << ",";
  os << err.vals(counte++) << "]";
  os << "}," << endl;

  os << "\"dpenergy\":{" << endl;
  os << "\"vals\":[";
  for(int p=0; p < nparms-1; p++) os << avg.vals(counta++) << ",";
  os << avg.vals(counta++) << "],";
  os << "\"err\":[";
  for(int p=0; p < nparms-1; p++) os << err.vals(counte++) << ",";
  os << err.vals(counte++) << "]";
  os << "}," << endl;

  
  Average_return tmpavg,tmperr;
  tmpavg.type="tbdm";
  tmpavg.vals.Resize(nmo+ndm_elements);
  tmperr.vals.Resize(nmo+ndm_elements);
  for(int d=0; d< nmo+ndm_elements; d++) {
    tmpavg.vals(d)=avg.vals(counta++);
    tmperr.vals(d)=err.vals(counte++);
  }
  dm_eval.jsonOutput(tmpavg,tmperr,os);
  os << ",";
  
  os << "\"dprdm\":[" << endl;
  for(int p=0; p < nparms; p++) {
    os << "{";
    for(int d=0;d < ndm_elements; d++) {
      tmpavg.vals(nmo+d)=avg.vals(counta++);
      tmperr.vals(nmo+d)=err.vals(counte++);
    }
    dm_eval.jsonOutput(tmpavg,tmperr,os);
    os << "}";
    if(p < nparms-1) os <<",";
  }
  os << "]" << endl;

  os << "}" << endl;

}
//-----------------------------------------------------------------------------

