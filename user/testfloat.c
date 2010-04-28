
#include <stdio.h>
#include <math.h>

int main()
{	
	printf("%f %f %f\n", 1.0, M_PI, M_E);
	printf("%.0f %#.0f %.0f\n", 1.0, M_PI, M_E);
	printf("%.3f %#.3f %.3f\n", 1.0, M_PI, M_E);
	printf("%.15f %#.15f %.15f\n", 1.0, M_PI, M_E);

	printf("%20f %20f %20f\n", 1.0, M_PI, M_E);
	printf("%20.0f %20#.0f %20.0f\n", 1.0, M_PI, M_E);
	printf("%20.3f %20#.3f %20.3f\n", 1.0, M_PI, M_E);
	printf("%20.15f %20#.15f %20.15f\n", 1.0, M_PI, M_E);

	printf("%f %f %f\n", 123456.123456123456, HUGE_VAL, -HUGE_VAL);

	int x,y,z;
	int rc = sscanf("-12345 0777 -0xffff", "%i%i%i", &x, &y, &z);
	printf("rc%d %d %o %x\n", rc, x, y, z);

	float a,b;
	double c,d;
	rc = sscanf("123456.123456 -.532 987654321.987654321 123",
			"%f%f%lf%lf", &a, &b, &c, &d);
	printf("rc%d %f %f %.15f %f\n", rc, a,b,c,d);

	char s1[10], s2[10], s3[10], s4[10];
	rc = sscanf("   abc  def  ghi  jkl  ", "%s%5s%2s%6c",
			&s1, &s2, &s3, &s4);
	printf("rc%d '%s' '%s' '%s' '%s'\n", rc, s1, s2, s3, s4);

	return 0;
}

