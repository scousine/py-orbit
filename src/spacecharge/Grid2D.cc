//This class repersents a 2D rectangular grid 

#include "Grid2D.hh"
#include "ParticleMacroSize.hh"

#include <iostream>

using namespace OrbitUtils;

// Constructor
Grid2D::Grid2D(int xSize, int ySize): CppPyWrapper(NULL)
{
	xSize_ = xSize;
	ySize_ = ySize;
	xMin_ = -1.0; 
	xMax_ = +1.0; 
	yMin_ = -1.0; 
	yMax_ = +1.0; 
	init();
	setZero();
}

Grid2D::Grid2D(int xSize, int ySize, 
	       double xMin, double xMax,    
	       double yMin, double yMax): CppPyWrapper(NULL)
{
	xSize_ = xSize;
	ySize_ = ySize; 
	xMin_ = xMin;
	xMax_ = xMax;
	yMin_ = yMin;
	yMax_ = yMax;
	init();
	setZero();
}

void Grid2D::init(){
	
  if( xSize_ < 3 || ySize_ < 3){
		int rank = 0;
		ORBIT_MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		if(rank == 0){
			std::cerr << "Grid2D::Grid2D - CONSTRUCTOR \n" 
         				<< "The grid size too small (should be more than 3)! \n" 
								<< "number x bins ="<< xSize_ <<" \n"
								<< "number y bins ="<< ySize_ <<" \n"
								<< "Stop. \n";
		}
		ORBIT_MPI_Finalize();
  }	
	
	dx_ = (xMax_ - xMin_)/(xSize_ -1);
	dy_ = (yMax_ - yMin_)/(ySize_ -1);
	arr_ = new double*[xSize_];
	for(int i = 0; i < xSize_; i++){
		arr_[i] = new double[ySize_];
	}
}

// Destructor
Grid2D::~Grid2D()
{
	//std::cerr<<"debug Grid2D::~Grid2D()"<<std::endl;
	for(int i = 0; i < xSize_; i++){
		delete [] arr_[i];
	}
	delete [] arr_;
}

/** Sets the value to the one point of the 2D grid  */	
void Grid2D::setValue(double value, int ix, int iy){
	arr_[ix][iy] = value;
}

/** Returns the value on grid*/
double Grid2D::getValueOnGrid(int ix, int iy){
	return arr_[ix][iy];
}

/** Returns the interpolated value from the 2D grid */	
double Grid2D::getValue(double x, double y){
	int iX, iY;
	double Wxm, Wx0, Wxp, Wym, Wy0, Wyp;
	double xFract,  yFract;
	double xFract2, yFract2;
	getIndAndFracX(x,iX,xFract);
	getIndAndFracY(y,iY,yFract);
	xFract2 = xFract * xFract;
	yFract2 = yFract * yFract;
	Wxm = 0.5 * (0.25 - xFract + xFract2);
	Wx0 = 0.75 - xFract2;
	Wxp = 0.5 * (0.25 + xFract + xFract2);
	Wym = 0.5 * (0.25 - yFract + yFract2);
	Wy0 = 0.75 - yFract2;
	Wyp = 0.5 * (0.25 + yFract + yFract2);
	double value =
	Wxm * Wym * arr_[iX-1][iY-1] +
	Wxm * Wy0 * arr_[iX-1][iY]   +
	Wxm * Wyp * arr_[iX-1][iY+1] +
	Wx0 * Wym * arr_[iX]  [iY-1] +
	Wx0 * Wy0 * arr_[iX]  [iY]   +
	Wx0 * Wyp * arr_[iX]  [iY+1] +
	Wxp * Wym * arr_[iX+1][iY-1] +
	Wxp * Wy0 * arr_[iX+1][iY]   +
	Wxp * Wyp * arr_[iX+1][iY+1]; 
	return value;
}	

/** Bins the Bunch into the 2D grid. If bunch has a macrosize particle attribute it will be used. */	
void Grid2D::binBunch(Bunch* bunch){
	bunch->compress();
	double** part_coord_arr = bunch->coordArr();
	int has_msize = bunch->hasParticleAttributes("macrosize");
	if(has_msize > 0){
		ParticleMacroSize* macroSizeAttr = (ParticleMacroSize*) bunch->getParticleAttributes("macrosize");
		double m_size = 0.;
		for(int i = 0, n = bunch->getSize(); i < n; i++){
			m_size = macroSizeAttr->macrosize(i);
			binValue(m_size,part_coord_arr[i][0],part_coord_arr[i][2]);
		}	
		return;
	}
	double m_size = bunch->getMacroSize();
	for(int i = 0, n = bunch->getSize(); i < n; i++){
		binValue(m_size,part_coord_arr[i][0],part_coord_arr[i][2]);	
	}
}

/** Bins the value into the 2D grid */	
void Grid2D::binValue(double value, double x, double y){
	if(x < xMin_ || x > xMax_ || y < yMin_ || y > yMax_) return;
	int iX, iY;
	double Wxm, Wx0, Wxp, Wym, Wy0, Wyp;
	double xFract,  yFract;
	double xFract2, yFract2;
	getIndAndFracX(x,iX,xFract);
	getIndAndFracY(y,iY,yFract);
	xFract2 = xFract * xFract;
	yFract2 = yFract * yFract;
	Wxm = 0.5 * (0.25 - xFract + xFract2);
	Wx0 = 0.75 - xFract2;
	Wxp = 0.5 * (0.25 + xFract + xFract2);
	Wym = 0.5 * (0.25 - yFract + yFract2);
	Wy0 = 0.75 - yFract2;
	Wyp = 0.5 * (0.25 + yFract + yFract2);
	arr_[iX-1][iY-1] += Wxm * Wym * value;      
  arr_[iX-1][iY]   += Wxm * Wy0 * value;   
  arr_[iX-1][iY+1] += Wxm * Wyp * value; 
  arr_[iX]  [iY-1] += Wx0 * Wym * value; 
  arr_[iX]  [iY]   += Wx0 * Wy0 * value; 
  arr_[iX]  [iY+1] += Wx0 * Wyp * value; 
  arr_[iX+1][iY-1] += Wxp * Wym * value; 
  arr_[iX+1][iY]   += Wxp * Wy0 * value; 
  arr_[iX+1][iY+1] += Wxp * Wyp * value;             
}

/** Calculates gradient at a position (x,y) */	
void Grid2D::calcGradient(double x, double y, double& ex, double& ey){
	double dWxm,dWx0,dWxp,dWym,dWy0,dWyp;
	double Wxm, Wx0, Wxp, Wym, Wy0, Wyp;
	int iX, iY;
	double xFract,  yFract;
	double xFract2, yFract2;
	getIndAndFracX(x,iX,xFract);
	getIndAndFracY(y,iY,yFract);	
	xFract2 = xFract * xFract;
	yFract2 = yFract * yFract;
	Wxm = 0.5 * (0.25 - xFract + xFract2);
	Wx0 = 0.75 - xFract2;
	Wxp = 0.5 * (0.25 + xFract + xFract2);
	Wym = 0.5 * (0.25 - yFract + yFract2);
	Wy0 = 0.75 - yFract2;
	Wyp = 0.5 * (0.25 + yFract + yFract2);	
  dWxm = (-1.0)*(0.5 - xFract); 
	dWx0 = (-1.0)*(-2.) * xFract; 
	dWxp = (-1.0)*(-(0.5 + xFract)); 
	dWym = (-1.0)*(0.5 - yFract); 
	dWy0 = (-1.0)*(+2.) * yFract; 
	dWyp = (-1.0)*(-(0.5 + yFract));
  ex =
	dWxm * Wym * arr_[iX-1][iY-1] +
	dWxm * Wy0 * arr_[iX-1][iY]   +
	dWxm * Wyp * arr_[iX-1][iY+1] +
	dWx0 * Wym * arr_[iX]  [iY-1] +
	dWx0 * Wy0 * arr_[iX]  [iY]   +
	dWx0 * Wyp * arr_[iX]  [iY+1] +
	dWxp * Wym * arr_[iX+1][iY-1] +
	dWxp * Wy0 * arr_[iX+1][iY]   +
	dWxp * Wyp * arr_[iX+1][iY+1]; 
	ex = ex / dx_;
  ey =
	Wxm * dWym * arr_[iX-1][iY-1] +
	Wxm * dWy0 * arr_[iX-1][iY]   +
	Wxm * dWyp * arr_[iX-1][iY+1] +
	Wx0 * dWym * arr_[iX]  [iY-1] +
	Wx0 * dWy0 * arr_[iX]  [iY]   +
	Wx0 * dWyp * arr_[iX]  [iY+1] +
	Wxp * dWym * arr_[iX+1][iY-1] +
	Wxp * dWy0 * arr_[iX+1][iY]   +
	Wxp * dWyp * arr_[iX+1][iY+1];
	ey = ey / dy_;
}

/** Calculates gradient at a grid point (ix,iy) */	
void Grid2D::calcGradient(int iX, int iY, double& ex, double& ey){
	if(iX != 0 && iX != (xSize_ - 1) && iY != 0 && iY != (ySize_ - 1)){
		ex =
		(- 0.125) * arr_[iX-1][iY-1] +
		(- 0.75 )* arr_[iX-1][iY]   +
		(- 0.125) * arr_[iX-1][iY+1] +
		0.125 * arr_[iX+1][iY-1] +
		0.75  * arr_[iX+1][iY]   +
		0.125 * arr_[iX+1][iY+1]; 	
		ex = 0.5 * ex / dx_;
		ey =
		(-0.125) * arr_[iX-1][iY-1] +
		( 0.125) * arr_[iX-1][iY+1] +
		(-0.75 ) * arr_[iX]  [iY-1] +
		( 0.75 ) * arr_[iX]  [iY+1] +
		(-0.125) * arr_[iX+1][iY-1] +
		( 0.125) * arr_[iX+1][iY+1];
		ey = 0.5 * ey / dy_;
		return;
	} 
	double x = xMin_ + iX*dx_;
	double y = yMin_ + iY*dy_;
	calcGradient(x,y,ex,ey);
}

/** Sets all grid points to zero */	
void Grid2D::setZero(){
	for(int i = 0; i < xSize_; i++){
		for(int j = 0; j < ySize_; j++){
			arr_[i][j] = 0.;
		}
	}
}

/** Returns the reference to the 2D array */	
double** Grid2D::getArr(){
	return arr_;
}
	
/** Returns the grid size in x-direction */
int Grid2D::getSizeX(){
	return xSize_;
}

/** Returns the grid size in y-direction */
int Grid2D::getSizeY(){
	return ySize_;
}

void Grid2D::getIndAndFracX(double x, int& ind, double& frac){
   ind  = int ( (x - xMin_)/dx_ + 0.5 );
   if(ind < 1) ind = 1;
   if(ind > (xSize_-2)) ind = xSize_ - 2;
   frac = (x - (xMin_ + ind*dx_))/dx_; 	
}

void Grid2D::getIndAndFracY(double y, int& ind, double& frac){
   ind  = int ( (y - yMin_)/dy_ + 0.5 );
   if(ind < 1) ind = 1;
   if(ind > (ySize_-2)) ind = ySize_ - 2;
   frac = (y - (yMin_ + ind*dy_))/dy_; 	
}	

/** Returns the grid point x-coordinate for this index. */   
double Grid2D::getGridX(int index){
	return xMin_ + index*dx_;
}

/** Returns the grid point y-coordinate for this index. */   
double Grid2D::getGridY(int index){
	return yMin_ + index*dy_;
}

/** Returns the grid step along x-axis */
double Grid2D::getStepX(){
	return dx_;
}

/** Returns the grid step along y-axis */
double Grid2D::getStepY(){
	return dy_;
}

/** Returns the max x in the grid points */ 
double Grid2D::getMaxX(){return xMax_;};

/** Returns the min x in the grid points */ 
double Grid2D::getMinX(){return xMin_;};

/** Returns the max y in the grid points */ 
double Grid2D::getMaxY(){return yMax_;};

/** Returns the min y in the grid points */ 
double Grid2D::getMinY(){return yMin_;};

/** Sets x-grid */
void Grid2D::setGridX(double xMin, double xMax){
	xMax_ = xMin;
	xMax_ = xMax;
	dx_ = (xMax_ - xMin_)/(xSize_ -1);
	setZero();
}

/** Sets y-grid */
void Grid2D::setGridY(double yMin, double yMax){
	yMax_ = yMin;
	yMax_ = yMax;
	dy_ = (yMax_ - yMin_)/(ySize_ -1);		
	setZero();
}

/** Returns 1 if (x,y) is inside the grid region, and 0 otherwise */
int Grid2D::isInside(double x,double y){
	if(x < xMin_ || x > xMax_) return 0;
	if(y < yMin_ || y > yMax_) return 0;
	return 1;
}
	
